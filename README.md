# NAME
**daytimed** -- Daytime Protocol daemon

# SYNOPSIS
**daytimed** [**-d**]

# DESCRIPTION
**daytimed** is a privilege-dropping server that implements the Daytime Protocol as specified in RFC 867.
It responds to TCP connections made on port *13*, returns a human-readable date string to the client,
then closes the connection.

The options are as follows:  

**-d** - Do not daemonize. **daytimed** will run in the foreground and output debug messages to stdout.

In debug mode (**-d**), root privileges are not required for startup and the server will listen on
*localhost* port *13013*.

# CAVEATS
**daytimed** only builds and works on OpenBSD 6.6 or newer, because it relies on pledge(2) and
the built-in make infrastructure. Portability could be achieved with minimal effort, but that
is outside the scope of my current goals.

# AUTHORS
Scott Bennett
