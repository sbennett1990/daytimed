# NAME
**daytimed** -- Daytime Protocol daemon

# SYNOPSIS
**daytimed** [**-d**]

# DESCRIPTION
**daytimed** is a server that implements the Daytime Protocol as specified in RFC 867. It responds to TCP
connections made on port *13* and returns a human-readable date string to the client then closes the
connection.

The options are as follows:  

**-d** - Do not daemonize. **daytimed** will run in the foreground and output debug messages to stdout.

In debug mode (**-d**), root privileges are not required and the server will listen on *localhost*
port *13013*.

# AUTHORS
Scott Bennett
