const express = require('express');
const basicAuth = require('express-basic-auth');
require('dotenv').config();

const app = express();

const browser_username = process.env.browser_username;
const browser_password = process.env.browser_password;

if (process.env.NODE_ENV === "LOCAL") {
  console.log("Starting local setup.");
} else if (process.env.NODE_ENV === "VM") {
  console.log("Starting VM setup.");
}

// this is the latest frame, we'll keep this updated whenever
// '/upload' is hit, and return that image whenever '/stream'
// is hit.
let latestFrame = null;

// the stream endpoint (user accessible) requires basic auth from the .env file
// and returns the jpeg image stored above once authenticated
app.get(
  "/stream",
  basicAuth({
    users: { [browser_username]: browser_password },
    challenge: true
  }),
  (req, res) => {
    if (!latestFrame) {
      return res.status(404).send("No frame yet");
    }
    res.set("Content-Type", "image/jpeg");
    res.send(latestFrame);
  }
);

// the upload endpoint (ESP32 accessible) requires an api key from the .env file
// and allows the caller to update the latestFrame with an uploaded jpg image
app.use('/upload', express.raw({ type: 'image/jpeg', limit: '2mb' })); 
app.post('/upload', (req, res) => {
  const auth = req.headers['authorization'];
  if (!auth || auth !== `Bearer ${esp32_api_key}`) { // this checks to make sure the correct api key is sent
    return res.status(401).send('Unauthorized');
  }

  latestFrame = req.body;
  res.send('Frame received');
});


const PORT = 8443;
app.listen(PORT, () => {
  console.log(`Server running on ${PORT}`);
});