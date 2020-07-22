# Version Control System

Where's The File (WTF) is a fully functional client-server version control system written in C. Through sockets, file I/O, and multithreading, it 
allows up to ten clients to interact with, push projects to, get projects from and otherwise modify a repository in a remote server. While a local 
version of the repository exists on the client-side, the server maintains the version most recently pushed while keeping track of the project's 
history. 

WTF supports the following commands: configure, checkout, update, upgrade, commit, push, create, destroy, add, remove, current version, histroy and 
rollback. Check out readme.pdf to understand the functionality more in-depth. 
