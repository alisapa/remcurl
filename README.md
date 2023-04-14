# remcurl --- CLI interface to the ReMarkable e-ink tablet

`remcurl` interacts with the HTTP interface exposed by the ReMarkable e-ink
tablet on `http://10.11.99.1/`. It supports listing, download (including
recursive download), and upload of files.

Note that this tool hasn't been tested nearly enough and probably will fail
on a lot of edge cases. It's not even good code, probably.  But it works well
enough to fulfill its purpose.

## Dependencies

- [libcurl](https://curl.se): for sending HTTP requests
- [cJSON](https://github.com/DaveGamble/cJSON): for parsing JSON responses

## Build and Install

```sh
make
make install
```

If any additional CFLAGS are needed, they can be passed in `EXTRA_CFLAGS`,
such as:
```sh
make EXTRA_CFLAGS=-g # build with debugging information
```

To change install prefix, use `INSTALL_PREFIX`, e.g.:
```sh
make INSTALL_PREFIX=/home/user/.local install # local install
```

It is possible to uninstall using
```sh
make uninstall
```
Be careful to specify the same prefix that was used for installing.

## Usage

See the manpage, available in this repository as `man/remcurl.1`. It can be
viewed as follows:
```sh
groff -Tascii -man man/remcurl.1 # output as text
groff -Tps -man man/remcurl.1    # output as PostScript
man man/remcurl.1                # view with man pager
```
