<h1 align="center">tolk</h1>

<p align="center">
    <img src="https://img.shields.io/badge/license-MIT-green?style=flat-square" alt="MIT License" />
  <img src="https://img.shields.io/github/last-commit/simon-danielsson/tolk/main?style=flat-square&color=blue" alt="Last commit" />
      <img src="https://img.shields.io/badge/C_version-99-cyan?style=flat-square" alt="ANSI C" />
</p>
  
<p align="center">
  <a href="#info">Info</a> •
  <a href="#install">Install</a> •
  <a href="#usage">Usage</a> •
  <a href="#license">License</a>
</p>  
  
---
<div id="info"></div>

## Info
  
A simple and configurable PS1 prompt written in C.
  
---
<div id="install"></div>

## Install
  
Clone this repo, compile with command `./build.sh release` and run the binary
with PS1 in your `.bashrc` or wherever you want in your shell path.
  
``` bash
git clone git@github.com:simon-danielsson/tolk.git
cd tolk
./build.sh -n -v release
# the compiled executable is now inside the `bin` directory
```
  
``` bash
# ~/.bashrc
export PS1='$(~/path/to/tolk/bin/tolk)'
```
  
---
<div id="usage"></div>
  
## Usage
   
### Configuration
  
To configure **tolk**, create a new `.tolk.ini` in your home directory (i.e
`$HOME`). Copy paste the contents of the [example configuration file](./.tolk.example.ini) to get started.
    
---
<div id="license"></div>
  
## License
  
This project is licensed under the [MIT License](https://github.com/simon-danielsson/tolk/blob/main/LICENSE).  
