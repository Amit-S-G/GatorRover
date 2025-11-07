const express = require('express');
const basicAuth = require('express-basic-auth');
const fs = require('fs')
const path = require('path');
require('dotenv').config();

const app = express();

const browser_username = process.env.browser_username;
const browser_password = process.env.browser_password;
const access_key = process.env.esp32_api_key;

const FRAMERATE = 2000;

if (process.env.NODE_ENV === "LOCAL") {
  console.log("Starting local setup.");
} else if (process.env.NODE_ENV === "VM") {
  console.log("Starting VM setup.");
}

// we're abstracting "basicAuth" to the top, because we're going to need
// to protect multiple things with it, so putting it here makes the code
// a little cleaner
const authMiddleware = basicAuth({
  users: { [browser_username]: browser_password },
  challenge: true
});

// this is the latest frame, we'll keep this updated whenever
// '/upload' is hit, and return that image whenever '/stream'
// is hit.
let latestFrame = null;

// this will expose everything in the public folder
// while being protected by auth middleware
//
// (we don't exactly need to protect the main page,
// but protecting the main page allows us to pass the credentials
// through when html makes the request for script.js)
app.use("/view", authMiddleware, express.static(path.join(__dirname, "public")));

// the stream endpoint (user accessible) requires basic auth from the .env file
// it sets up an MJPEG stream which the frontend index.html connects to
// where the latest image is streamed forward every FRAMERATE/1000 seconds.
// framerate should be set to a high value in testing to avoid bandwidth waste.
app.get(
  "/stream",
  authMiddleware,
  (req, res) => {
  if (!latestFrame) return res.status(404).send("No frame yet");

  // set the headers for an MJPEG stream
  res.writeHead(200, {
    "Cache-Control": "no-cache",
    "Connection": "close",
    "Content-Type": "multipart/x-mixed-replace; boundary=frame"
  });

  const sendFrame = () => {
    if (latestFrame) {
      res.write("--frame\r\n");
      res.write("Content-Type: image/jpeg\r\n\r\n");
      res.write(latestFrame);
      res.write("\r\n");
    }
  };

  // Send a new frame every FRAMERATEms
  const interval = setInterval(sendFrame, FRAMERATE);

  // stop sending frames if the client disconnects
  req.on("close", () => {
    clearInterval(interval);
  });
});

// the upload endpoint (ESP32 accessible) requires an api key from the .env file
// and allows the caller to update the latestFrame with an uploaded jpg image
app.use('/upload', express.raw({ type: 'image/jpeg', limit: '2mb' })); 
app.post('/upload', (req, res) => {
  const auth = req.headers['authorization'];
  if (!auth || auth !== `Bearer ${access_key}`) { // this checks to make sure the correct api key is sent
    return res.status(401).send('Unauthorized');
  }

  latestFrame = req.body;
  res.send('Frame received');
});


const PORT = 8443;
app.listen(PORT, () => {
  console.log(`Server running on ${PORT}`);
});
app.get('/', (req, res) => {
  res.redirect('/view');
});
