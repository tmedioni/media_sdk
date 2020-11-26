### Install and build

You will need libssl on your machine.

```sh
$git clone git@github.com:tmedioni/media_sdk.git
$mkdir build && cd build && cmake ..
$make
```

### Dependencies

Two external dependencies are used.

[spdlog](https://github.com/gabime/spdlog) is used to manage logging to console.
[httplib](https://github.com/yhirose/cpp-httplib) is used both to serve and request content over http.

### Running

This piece of software act as a proxy between a CDN and your video client (vlc as an example).

You need to provide the address of the CDN as a command line argument when you launch the server. If you want to watch, let's say, `https://bitdash-a.akamaihd.net/content/sintel/hls/playlist.m3u8`

Then run the program with:

```sh
./media-sdk https://bitdash-a.akamaihd.net
```

It will listen on port 8080.

On Vlc, open a NetworkStream with the following adress: `http://localhost:8080/content/sintel/hls/playlist.m3u8`

media_sdk will act as a proxy between vlc and the CDN, and for each request vlc makes it will display the time for the go-around to the CDN.
