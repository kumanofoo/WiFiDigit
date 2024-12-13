//! Parse the configuration file *keepipe.toml*.

use serde::{Deserialize, Serialize};
use std::error::Error;
use std::fs;
use std::fs::File;
use std::io::Write;
use std::path::PathBuf;

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

/// The filename of JSON format file.
#[derive(Debug, Deserialize, Clone)]
pub struct ExportToJson {
    pub filename: String,
}
impl ExportToJson {
    pub fn export(&self, highest: f32, lowest: f32) -> Result<(), Box<dyn Error>> {
        let temp_peek = ExportTemperature { highest, lowest };
        let temp_peek_json = serde_json::to_string_pretty(&temp_peek)?;
        let mut file = File::create(&self.filename)?;
        file.write_all(temp_peek_json.as_bytes())?;

        Ok(())
    }
}

#[derive(Debug, Deserialize, Serialize, Clone)]
pub struct ExportTemperature {
    pub highest: f32,
    pub lowest: f32,
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
///
/// [json]
/// filename = "./output.json"
/// ```
///
/// Read `keepipe.toml` and print it.
/// ```no_run
/// use keepipe::config::Config;
/// use std::path::PathBuf;
///
/// fn main() {
///     let config = Config::new(&PathBuf::from("keepipe.toml")).unwrap();
///     println!("{:?}", config);
/// }
/// ```
#[derive(Debug, Deserialize, Clone)]
pub struct Config {
    pub area: AreaCode,
    pub wifidigit: WiFiDigit,
    pub actions: Vec<Action>,
    pub json: Option<ExportToJson>,
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

#[cfg(test)]
mod tests {
    use std::io::BufReader;
    use std::os::unix::fs::PermissionsExt;

    use super::*;

    #[test]
    fn no_area() {
        let mut path = PathBuf::new();
        path.push("./tests/no_area.toml");
        let cfg = Config::new(&path);
        assert!(cfg.is_err());
    }

    #[test]
    fn no_json() {
        let mut path = PathBuf::new();
        path.push("./tests/no_json.toml");
        let cfg = Config::new(&path).unwrap();
        assert!(cfg.json.is_none());
    }

    #[test]
    fn export_to_json() {
        let mut path = PathBuf::new();
        path.push("./tests/export_to_json.toml");
        let cfg = Config::new(&path).unwrap();
        let json_file = cfg.json.unwrap();
        assert_eq!(
            json_file.filename,
            "tests/export_to_json_output.json".to_string()
        );
        let _ = json_file.export(30.1, -5.1);

        let file = File::open(&json_file.filename).unwrap();
        let reader = BufReader::new(file);
        let result: ExportTemperature = serde_json::from_reader(reader).unwrap();
        assert_eq!(result.highest, 30.1);
        assert_eq!(result.lowest, -5.1);

        fs::remove_file(&json_file.filename).unwrap();
    }

    #[test]
    fn fail_to_export() {
        let cfg = Config::new(&PathBuf::from("./tests/fail_to_export.toml")).unwrap();
        let json_file = cfg.json.unwrap();
        assert_eq!(json_file.filename, "tests/fail_to_export.json".to_string());
        assert!(json_file.export(30.1, -5.1).is_ok());

        fs::set_permissions(&json_file.filename, PermissionsExt::from_mode(0o400)).unwrap();

        let file = File::open(&json_file.filename).unwrap();
        let reader = BufReader::new(file);
        let result: ExportTemperature = serde_json::from_reader(reader).unwrap();
        assert_eq!(result.highest, 30.1);
        assert_eq!(result.lowest, -5.1);

        let _ = json_file.export(300.1, -50.1).unwrap_or_else(|why| {
            assert_eq!(
                "failed to export to tests/fail_to_export.json: Permission denied (os error 13)",
                format!(
                    "failed to export to {}: {}",
                    json_file.filename,
                    why.to_string()
                )
            );
        });

        fs::set_permissions(&json_file.filename, PermissionsExt::from_mode(0o600)).unwrap();
        fs::remove_file(&json_file.filename).unwrap();
    }
}
