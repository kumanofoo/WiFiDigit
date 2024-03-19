# Keepipe
Keepipe, which is one of WiFiDigit clients, display the lowest or highest temperature
on the LED matrix on Arduino Uno R4 WiFi.
It will notify you of the possibility of water pipe ⛄️freezing🧊 or the risk of 🌞heatstroke🥵.

This app fetches the temperature from [Japan Meteorological Agency](https://www.jma.go.jp/) and
sends them to WiFiDigit over HTTP.

## Requirements
### crate
- chrono(0.4)
- env_logger(0.10)
- log(0.4)
- clap(4.4)
- serde(1.0)
- serde_json(1.0)
- toml(0.7)
- reqwest(0.11)

## Configurations
### Target area codes (*jma_offices* and *jma_area_code*)
You can get the area code from https://www.jma.go.jp/bosai/common/const/area.json.
The codes of prefecture or area are in "offices" key in the *area.json*, for example, "Ishikari Sorachi Shiribeshi" in Hokkaido is 016000.

With the office code, you can retrieve the weather forecast in the prefecture or area from
https:<i>//</i>www.jma.go.jp/bosai/forecast/data/forecast/{OFFICES_CODE}.json,
for example, https://www.jma.go.jp/bosai/forecast/data/forecast/016000.json.

The area code is in "0/timeSeries/2/areas/{*area_index*}/area/code", for example, "札幌(Sapporo)" is 14163.

### keepipe.toml
Default configuration file is 'keepipe.toml' in current directory.
```TOML
[area]
jma_offices = "016000"   # offices code (Ishikari Sorachi Shiribeshi)
jma_area_code = "14163"  # area code (Sapporo)
reference_time = 5       # If data is collected between midnight and 4:59 a.m.,
                         # the temperature for the same day will be used;
                         # if between 5:00 a.m. and 11:59 p.m.,
                         # the temperature for the next day will be used. 

[wifidigit]
url = "http://example.com"  # URL to a Arduino with WiFiDigit running.

# 1. Doesn't display upside down.
#    $ curl -X PUT "http://example.com/upside-down/false"
[[actions]]
method = "PUT"
command = "upside-down/false"

# 2. Display lowest temperature.
#    $ curl -X PUT "http://example.com/2digit/{tempLowest}"
#    '{tempLowest}' is replaced with weather forecast lowest temperature.
[[actions]]
method = "PUT"
command = "2digit/{tempLowest}"

# 3. Hourly bar
#    $ curl -X PUT "http://example.com/bar/{nowHour}"
#    '{nowHour}' is replaced with current time hour.
[[actions]]
method = "PUT"
command = "bar/{nowHour}"

# 4. Blink with 1 sec on and 10 msec off if the temperature is -2 degrees Celsius or lower.
#    $ curl -X PUT "http://example.com/blink-flipped/1000.10"
[[actions]]
lowest = -2
method = "PUT"
command = "blink-flipped/1000.10"
```

### Keys to trigger the action
- *lowest*: if the temerature is *lowest* degrees Celsius or lower.
- *highest*: if the temerature is *highest* degrees Celsius or higher.

When there are no triggers in [[actions]], the action is always executed.

When there are some triggers, the action is executed only when the all of them are activated.

### Keywords
Some keywords are replaced with variables
- *{tempLowest}*: Lowest temperature.
- *{tempHighest}*: Highest temperature. 
- *{nowHour}*: current time hour.
- *{reportHour}*: report hour of weather forecast.

## Build and Install
### Build
```Shell
$ git clone https://gitnub.com/kumanofoo/WiFiDigit.git
$ cd WiFiDigit/keepipe
$ cargo build --release
```
### Run
```Shell
$ cargo run --release
```
### Install
```Shell
$ sudo mkdir /opt/keepipe
$ sudo cp target/release/keepipe /opt/keepipe
$ sudo cp keepipe.toml /opt/keepipe
```
And then edit /opt/keepipe/keepipe.toml.

### cron
Open crontab:
```Shell
$ crontab -e
```

and then edit crontab:
```Shell
# m h dom mon dow command
# Start new job 10 minutes of every hour: 1:10, 2:10, ...
10 * * * * /opt/keepipe/keepipe -c /opt/keepipe/keepipe.toml > /dev/null
```
