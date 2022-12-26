# irc

A basic IRC client and server, written in C. Supports a core subset of IRC functionality, including 
channels, nicknames, and private messages.

To build the programs, run `make` in the project directory. A build folder will created containing
`irc` and `ircd` along with some object files. The server can be with `./build/ircd` and the client
can be run with `./build/irc <username>`.

The following IRC commands are implemented. Some parameters may not be supported and there are 
some quirks (e.g., a channel must begin with a "#" in a JOIN command).

- NICK
- JOIN
- AWAY
- LIST
- KICK
- PART
- PRIVMSG
- QUIT

**NOTE**: There are some bugs with networking (e.g., messages may be received slighly corrupted).
