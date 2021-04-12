sudo apt-get update -y
sudo apt-get upgrade -y
sudo apt-get install python3 python3-pip python3-venv -y
sudo apt-get install g++ g++-mingw-w64 gdb -y 
sudo apt-get install ffmpeg -y
sudo apt-get install libvlccore-dev libvlc-dev vlc -y
# sudo apt-get install ffmpeg libavcodec-dev libavformat-dev libswscale-dev -y
sudo apt-get install libgl1-mesa-dev libxrandr-dev libopenal1 -y
sudo apt-get install libatlas3-base libopenblas-dev -y
sudo apt-get install expect -y

echo "export DISPLAY=:0" >> ~/.bashrc
echo "export PULSE_SERVER=tcp:127.0.0.1" >> ~/.bashrc
echo "export LIBGL_ALWAYS_INDIRECT=Yes" >> ~/.bashrc
echo "alias please='sudo \$(fc -ln -1)'" >> ~/.bashrc