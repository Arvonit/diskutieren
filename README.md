# chat

A chat client and server written in C.

## To-Do's

### Server

- [ ] Support IRC-type channels
- [ ] Maybe support DMs

### Client

- [ ] Figure out how to kill send thread when receive thread dies (it is waiting on stdin)
- [ ] Put cursor at the bottom of the terminal window (look at ncurses)
- [ ] Use GNU readline to support line editing in stdin
- [ ] Terminate sending thread when server has shut down
