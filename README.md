# MuteBot

Yet another simple Telegram bot that mutes new members in groups. Support joining by invite links, requests or @ usernames.

It is built upon TDLib C interface using C11.

*It may be the first project actually taking TDLib C interface into production. You can use this project as an example of how to use TDLib C interface.*

## Building

*Why do you need to spend half a hour, gigabytes of memory and gigabytes of disk to build TDLib again? Because TDLib doesn't support dynamic linking.*

```shell
$ mkdir cmake-build-release
$ cd cmake-build-release
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make -j mutebot
```

## Usage

Obtain API ID and API Hash from [https://my.telegram.org](https://my.telegram.org).

Obtain a bot token using BotFather.

```
$ export TD_API_ID <Your API ID>
$ export TD_API_HASH <Your API Hash>
$ export TD_BOT_TOKEN <Your Bot Token>
$ ./mutebot
```

Alternatively, you can replace the following environment variables with -i -H and -T command line options, but they are not recommended, because some operating systems expose command line arguments to all users.

### Configuration

Currently, there are only a few options for you:

* `-t`: Use test DC. (Default: false)
* `-d`: Specify TDLib data directory. (Default: `./td_data/`)
* `-l`: Logout of the current bot. See the following section.

### Log out

TDLib treats bots as normal accounts, so you need to log out if you want to change the bot token. To simplify your logout process, MuteBot will automatically logout your bot if you send SIGINT or SIGTERM to the daemon. Therefore, you can safely specify a new bot token at the next startup without worrying that it is still using the old one. However, you may still need to logout manually using the `-l` switch. It does nothing except for logging out the current bot session, if any.

# License

WTFPL

by Yuuta Liang <yuuta@yuuta.moe>