This repository contains project code for the GatorRover (name subject to change), a remote-controlled land rover designed for remote observation of disaster zones and other such high-risk areas where direct human action may be too dangerous. This project is developed by Sunny Gupta, Corey Phillips, Chenfeng Su, and Michael Yao.

## Requires
- Node v18.20.4+
- npm v9.2.0+

## Note
Currently, the code for the camera and the code for the motors are two separate files, while we work on merging them together. They can be found in `temps/camera` and `temps/motors`.

## Running the Project: Server
1. Clone the repo.
```git clone https://github.com/Amit-S-G/GatorRover.git```

2. Enter the server folder.
```cd GatorRover/server```

3. Install necessary packages.
```npm i```

4. Run the server.

Locally: 
```npm run start-local```

From the VPS:
```npm run start-vm```

### Missing Files: `.env`
In order to run the server, you will need an environment file in `server/` with the following form:
```
browser_username= ...
browser_password= ...
certs_path_vm= ...
certs_path_local= ...
esp32_api_key= ...
```