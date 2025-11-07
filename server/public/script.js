const img = document.getElementById("camera");
const statusText = document.getElementById("status");

img.onload = () => {
  statusText.textContent = "Visual link established!";
  statusText.style.color = "#00ff99";
};

img.onerror = () => {
  statusText.textContent = "No signal detected!";
  statusText.style.color = "#ff4444";
};
