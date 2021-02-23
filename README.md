# SEEsh: A UNIX shell

Ashley Van Spankeren

Contains the following built-in commands:
* cd [dir] - changes the working directory to dir
* pwd - prints the working directory
* help - displays information about the built-in commands
* exit - terminates SEEsh
* set var [value] - updates the value of var if it exists, or creates a new var with contents of value (or an empty string if value is omitted)
* unset var - destroys the environment variable var

Many thanks to Stephen Brennan (https://brennan.io/2015/01/16/write-a-shell-in-c/) as the tutorial made it simple to create the skeleton of SEEsh.

Known bugs:
* if both var and value are omitted, SEEsh returns an error rather than displaying all environment variables and values
