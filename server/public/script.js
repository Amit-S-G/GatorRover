const img = document.getElementById("camera");

// Poll /frame every 200ms
setInterval(() => {
  // the use of ?t= does nothing for the stream endpoint
  // however, it is implemented to prevent cacheing from
  // causing stream issues.
  img.src = "/stream?t=" + new Date().getTime();
}, 5000);