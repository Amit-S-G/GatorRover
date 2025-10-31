const fs = require('fs');
const https = require('https');
const express = require('express');
require('dotenv').config();

const app = express();
var cert_path = null;
const browser_username = process.env.browser_username;
const browser_password = process.env.browser_password;

if (process.env.NODE_ENV === "LOCAL"){
    console.log("Starting local setup.")
    cert_path = process.env.certs_path_local
}
else if(process.env.NODE_ENV === "VM"){
    console.log("Starting VM setup.")
    cert_path = process.env.certs_path_vm
}

const httpsOptions = {
  key: fs.readFileSync(cert_path + "/server.key"),
  cert: fs.readFileSync(cert_path + "/server.crt"),
  ca: fs.readFileSync(cert_path + "/ca.crt"),
  requestCert: true,
  rejectUnauthorized: false
};

app.get('/', (req, res) => {
  res.send('Hello, secure world!');
});

https.createServer(httpsOptions, app).listen(443, () => {
  console.log('Successfully started!');
});