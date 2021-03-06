## envoy

<a href="https://scan.coverity.com/projects/5987">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/5987/badge.svg"/>
</a>

Envoy helps you to manage ssh keys in similar fashion to [keychain], but
done in c, takes advantage of cgroups and systemd.

The daemon, `envoyd`, starts the agent of choice in a sanitized
environment and caches the associated environmental variables in memory.
The agent is started on demand and it's lifetime is tracked through
cgroups for accuracy. `envoyd` is typically started as root and can thus
serve all the users on the system at once. It checks the credentials of
the incoming connection and starts the agent under that uid/guid. If its
started as a user it will only be able to serve that particular user's
requests.

The `envoy` command connects to the daemon and gets all the information
associated with the current running agent. It can then do things like
add new keys to the agent or output shell code to inject these variables
into a shell.

This effectively allows a user to share a single long-running
authentication agent between all shells and sessions in a clean and
managed fashion that doesn't clutter user login sessions.

  [keychain]: http://www.funtoo.org/Keychain

### Setup

To setup envoy, first enable the socket:

    # systemctl enable envoy@ssh-agent.socket    # to make ssh-agent the default agent
    # systemctl enable envoy@gpg-agent.socket    # or to make it gpg-agent

Then add the following to your shell's rc file.

    envoy [key ...]
    source <(envoy -p)

The `-t` flag lets you override the default agent. So `envoy -t
gpg-agent` will launch gpg-agent even if ssh-agent is the system
default.

The envoyd daemon will also run just fine under a user session, just
note that it won't be able to serve multiple users at once in this
configuration.

### Usage

    usage: envoy [options] [key ...]
    Options:
     -h, --help            display this help
     -v, --version         display version
     -d, --defer           defer adding keys until the next envoy invocation
     -a, --add             add private key identities
     -x, --expunge         remove private key identities
     -k, --kill            kill the running agent
     -r, --reload          reload the agent (gpg-agent only)
     -l, --list            list fingerprints of all loaded identities
     -u, --unlock=[PASS]   unlock the agent's keyring (gpg-agent only)
     -p, --print           print out environmental arguments
     -s, --sh              print sh style commands
     -c, --csh             print csh style commands
     -f, --fish            print fish style commands
     -t, --agent=AGENT     set the preferred agent to start

Note that when passing in keys, if they reside in `~/.ssh/`, then just
providing the filename is sufficient.

### Envoy with ssh-agent

When invoking `envoy` causes `ssh-agent` to start, on that first run
any keys passed to `envoy` will be added to the agent. Without any
arguments, it'll try to add `.ssh/id_rsa`, `.ssh/id_dsa`, and
`.ssh/id_ecdsa` automatically.

### Envoy with gpg-agent

Keys are never implicitly added with `gpg-agent`. Instead, keys have to
be explicitly added through either `envoy -a` or `ssh-add`. The agent
will then continue track those identities automatically without having
to be specified in the future.

The agent will also still respect `~/.gnupg/gpg-agent.conf`. For
example, to disable scdaemon, put `disable-scdaemon` in that file.

Note that invoking envoy also updates gpg-agent with the current status,
if available, of the tty and X. It is the same effect of running `echo
UPDATESTARTUPTTY | gpg-connect-agent`. This may cause some odd behaviour
with the pinentry. The pinentry may appear in an inappropriate place if
this data becomes stale. This is a limitation of gpg-agent itself.

### Envoy's pam integration

Envoy provides a pam module to load the agent into the environment at
login instead of relying on `envoy -p`. To use it, edit
`/etc/pam.d/login` and add:

    session   optional    pam_envoy.so

Its also possible provide an optional argument to choose which agent
type to start:

    session   optional    pam_envoy.so    gpg-agent

Envoy can also optionally unlock gpg-agent's keyring automatically with
your password, but in order to do so it needs an auth token. To enable
this, add:

    auth      optional    pam_envoy.so
    session   optional    pam_envoy.so

Note that this relies on gpg-agent's passphrase presetting support. To
enable this, ensure `allow-preset-passphrase` is also in
`~/.gnupg/gpg-agent.conf`.

### Wrappers with envoy

Envoy has support for wrapping commands through `envoy-exec`. The
utility will connect to the daemon, setup the environment, and launch
the provided command. For example:

    envoy-exec ssh git@github.com

It is also possible to write an `envoy-exec` "script" to provide a
terser wrapper.

    #!/usr/bin/envoy-exec
    /usr/bin/ssh

This script will behave as if its been invoked as `envoy-exec ssh`.

### Cgroups support

Having been unable to find a simple cgroups library targeted at
embedding, I wrote my own. `cgroups.c` has been borrowed from my own
project [here][cgroups].

Any bugs with the cgroups support or confusions with terminology (I'm
pretty sure my terminology is way off) should be reported there.

  [cgroups]: https://github.com/vodik/clique
