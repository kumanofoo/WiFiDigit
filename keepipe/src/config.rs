//! Parse the configuration file *keepipe.toml*.
 
use serde::Deserialize;
use std::path::PathBuf;
use std::fs;

/// The area code of weather forecast that you want to retrieve from JMA.
/// 
/// See [Forecast](crate::forecast::Forecast) in more detail.
#[derive(Debug, Deserialize, Clone)]
pub struct AreaCode {
    pub jma_offices: String,
    pub jma_area_code: String,
    pub reference_time: Option<u32>,
}

/// The configurations of WiFiDigit server.
#[derive(Debug, Deserialize, Clone)]
pub struct WiFiDigit {
    pub url: String,
}

/// The request to WiFiDigit and its trigger.
#[derive(Debug, Deserialize, Clone)]
pub struct Action {
    pub lowest: Option<f32>,
    pub highest: Option<f32>,
    pub method: Option<String>,
    pub command: String,
}

/// Configuration TOML
/// 
/// # Example
/// `keepipe.toml`
/// ```TOML
/// [area]
/// jma_offices = "016000"   # Ishikari Sorachi Shiribeshi
/// jma_area_code = "14163"  # Sapporo
/// reference_time = 5       # If data is collected between midnight and 4:59 a.m.,
///                          # the temperature for the same day will be used;
///                          # if between 5:00 a.m. and 11:59 p.m.,
///                          # the temperature for the next day will be used.
/// 
/// [wifidigit]
/// url = "http://example.com"  # URL to a Arduino with WiFiDigit running.
/// 
/// # Doesn't display upside down.
/// [[actions]]
/// method = "PUT"
/// command = "upside-down/false"
/// 
/// # Display lowest temperture
/// [[actions]]
/// method = "PUT"
/// command = "2digit/{tempLowest}"
/// 
/// # Display Hourly bar
/// [[actions]]
/// method = "PUT"
/// command = "bar/{nowHour}"
/// 
/// # Blink with 1 sec on and 10 msec off if the temperature is -2 degrees Celsius or lower.
/// [[actions]]
/// lowest = -2
/// method = "PUT"
/// command = "blink-flipped/1000.10"
/// ```
/// 
/// Read `keepipe.toml` and print it.
/// ```no_run
/// use Config;
/// 
/// fn main() {
///     let config = Config::new(String::from("keepipe.toml")).unwrap();
///     println!("{:?}", config);
/// }
/// ```
#[derive(Debug, Deserialize, Clone)]
pub struct Config {
    pub area: AreaCode,
    pub wifidigit: WiFiDigit,
    pub actions: Vec<Action>,
}

impl Config {
    /// Read a TOML file `filename` and store each values to [Config].
    pub fn new(filename: &PathBuf) -> Result<Config, String> {
        let config_file: String = match fs::read_to_string(filename) {
            Ok(c) => c,
            Err(why) => return Err(why.to_string()),
        };
        let config: Config = match toml::de::from_str(&config_file) {
            Ok(c) => c,
            Err(why) => return Err(why.to_string()),
        };
        Ok(config)
    }    
}