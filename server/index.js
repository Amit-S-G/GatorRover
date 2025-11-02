const express = require('express');
const basicAuth = require('express-basic-auth');
const fs = require('fs')
const path = require('path');
require('dotenv').config();

const app = express();

const browser_username = process.env.browser_username;
const browser_password = process.env.browser_password;
const access_key = process.env.esp32_api_key;

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
let latestFrame = fs.readFileSync("Cute_dog.jpg");

// this will expose everything in the public folder
// while being protected by auth middleware
//
// (we don't exactly need to protect the main page,
// but protecting the main page allows us to pass the credentials
// through when html makes the request for script.js)
app.use("/view", authMiddleware, express.static(path.join(__dirname, "public")));

// the stream endpoint (user accessible) requires basic auth from the .env file
// and returns the jpeg image stored above once authenticated
app.get(
  "/stream",
  authMiddleware,
  (req, res) => {
  if (!latestFrame) return res.status(404).send("No frame yet");

  // this hashing logic hashes the latest image
  // and uses the "etag" trait of browsers to prevent
  // the image from sending again if it hasn't changed
  // (to stop my ngrok bandwidth from getting nuked)
  const etag = require('crypto')
    .createHash('md5')
    .update(latestFrame)
    .digest('hex');

  res.set('ETag', etag);

  if (req.headers['if-none-match'] === etag) {
    // the browser automatically sends the last etag it got
    // which we check here, and send 304 (no change) if the etags (md5 hashes) match
    return res.status(304).end(); 
  }

  res.set("Content-Type", "image/jpeg");
  res.send(latestFrame);
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