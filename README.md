# Ns3-Gym for 802.11ah
In this project, it was adapted the Ns3-gym (created for the last NS3 version) to be compatible with the HaLow networks simulator.  

# Ns-3
[Ns–3](https://www.nsnam.org/) is a discrete-event network simulator for Internet systems, targeted primarily for research and educational use. ns-3 is free software, licensed under the GNU GPLv2 license, and is publicly available for research, development, and use
# Ns3-Gym
[OpenAI Gym](https://gym.openai.com/) is a toolkit for reinforcement learning (RL) widely used in research. The network simulator [Ns–3](https://www.nsnam.org/) is the de-facto standard for academic and industry studies in the areas of networking protocols and communication technologies. [Ns3-Gym](https://apps.nsnam.org/app/ns3-gym/) is a framework that integrates both [OpenAI Gym](https://gym.openai.com/) and [Ns3-Gym](https://apps.nsnam.org/app/ns3-gym/) in order to encourage usage of RL in networking research.

# Ns3-Gym installation instructions (Ubuntu 18.04.4 LTS)
## 1. Prepare all dependecies:

**Option 1:** Follow the instructions on https://www.nsnam.org/wiki/Installation 

**Option 2:** Create an installation script as follows:
* Create a script file and open it for editing:
```
touch dependecies-installer.sh
nano -w dependecies-installer.sh
```
* Copy and paste the following a then save it:
```
#!/bin/sh
sudo apt-get update
sudo apt-get install gcc g++ python python3 -y
sudo apt-get install gcc g++ python python3 python3-dev -y
sudo apt-get install python3-setuptools git mercurial -y
sudo apt-get install qt5-default mercurial -y
sudo apt-get install gir1.2-goocanvas-2.0 python-gi python-gi-cairo python-pygraphviz python3-gi python3-gi-cairo python3-pygraphviz gir1.2-gtk-3.0 ipython ipython3 -y
sudo apt-get install openmpi-bin openmpi-common openmpi-doc libopenmpi-dev -y
sudo apt-get install autoconf cvs bzr unrar -y
sudo apt-get install gdb valgrind -y
sudo apt-get install uncrustify -y
sudo apt-get install doxygen graphviz imagemagick -y
sudo apt-get install texlive texlive-extra-utils texlive-latex-extra texlive-font-utils dvipng latexmk -y
sudo apt-get install python3-sphinx dia -y
sudo apt-get install gsl-bin libgsl-dev libgsl23 libgslcblas0 -y
sudo apt-get install tcpdump -y
sudo apt-get install sqlite sqlite3 libsqlite3-dev -y
sudo apt-get install libxml2 libxml2-dev -y
sudo apt-get install python3-pip -y
sudo apt-get install cmake libc6-dev libc6-dev-i386 libclang-6.0-dev llvm-6.0-dev automake phyton3-pip -y
python3 -m pip install --user cxxfilt
sudo apt-get install libgtk2.0-0 libgtk2.0-dev -y
sudo apt-get install vtun lxc uml-utilities -y
sudo apt-get install libboost-signals-dev libboost-filesystem-dev -y
pip3 install --upgrade pip
sudo apt-get install libzmq5 libzmq5-dev -y
sudo apt-get install libprotobuf-dev -y
sudo apt-get install protobuf-compiler -y
```
* Update the permissions of the file and then run it:
```
sudo chmod +x dependecies-installer.sh
bash dependecies-installer.sh
```
## 2. Install the ns3 Simulator:
* Download or clone the repository:
```
cd ~
git clone https://github.com/ingertb/ns3_HaLow_gym.git
```
* Configure and Build Ns-3 project:
```
cd <ns-3-folder>
CXXFLAGS="-std=c++11" ./waf configure --disable-examples --disable-tests
./waf
cp build/lib/libns3-dev-opengym-debug.so build/
```
## 3. Install the OpenGym module:
* Install ns3gym Python module.
```
cd <ns-3-folder>/contrib/opengym/
sudo pip3 install ./model/ns3gym
```
## 4. Test the simulator (Optional):

* Test the network simulator by  Running the hello-simulator simulation:
```
cd <ns-3-folder>
cp examples/tutorial/hello-simulator.cc scratch/
./waf --run scratch/hello-simulator
```
* Test the network simulator and the OpenGym module by  Running the opengym simulation:
1. **Run the simulation:** Open one terminal and then run:
```
cd <ns-3-folder>
./waf --run scratch/opengym/opengym
```
2. **Run the agent:** Open a second one terminal and then run::
```
cd <ns-3-folder>/scratch/opengym/
python3 ./test.py --start=0
```
