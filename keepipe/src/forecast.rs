//! Get the lowest or hightest temperature from Japan Meteorological Agency.

use log::{warn, error};
use chrono::{DateTime, Duration, Local, Timelike};


const API: &str = "https://www.jma.go.jp/bosai/forecast/data/forecast/";

/// Get the lowest or highest temperature from [Japan Meteorological Agency](https://www.jma.go.jp/).
/// 
/// The temperatures is in:
/// > https://www.jma.go.jp/bosai/forecast/data/forecast/{*offices*}.json
/// 
/// {*offices*} means an area in Japan. It is defined in:
/// > <https://www.jma.go.jp/bosai/common/const/area.json>
///
/// For example, 016000 is 'Ishikari Sorachi Shiribeshi' area. You can get the weather forecast in its area:
/// ```
/// $ curl https://www.jma.go.jp/bosai/forecast/data/forecast/016000.json
/// ```
///  
/// The path of the target temperature in the JSON is '0/timeSeries/2/areas/{*area index*}/temps'.
/// {*area index*} is the index of an sub area in the *offices*. 
/// For example, 0 is '札幌'(Sapporo) area and the *area code* is 14163 in 016000.json:
/// ```
/// # 016000.json
/// [
///   {
///     publishingOffice: "札幌管区気象台",
///     reportDatetime: "2024-01-23T05:00:00+09:00",
///     timeSeries: [ {...}, {...}, {
///       timeDefines: [...],
///       areas: [
///         {
///            area: {
///              name: "札幌",
///              code: "14163"
///            },
///            temps: ["4", "4", "-1", "1"]
///         },
///         {...}, {...},
///       ]
///     }]
///   },
///   {...}
/// ]
/// ```
/// 
/// # Examples
/// Print the lowest and highest temperature in Sapporo.
/// 
/// ```no_run
/// use Forecast;
/// 
/// fn main() {
///     let mut f = Forecast::new(String::from("016000"), String::from("14163"), 5);
///     f.update_temperature();
///     println!("report datetime: {}", f.get_report_datetime().unwrap());
///     println!("lowest: {}", f.temp_lowest.unwrap());
///     println!("highest: {}", f.temp_highest.unwrap());
/// }
/// ```
pub struct Forecast {
    pub offices: String,
    pub area_name: String,
    pub area_code: String,
    pub reference_time: u32,
    pub temp_lowest: Option<f32>,
    pub temp_highest: Option<f32>,
    pub update: DateTime<Local>,
    pub forecast: Option<serde_json::Value>,
}

impl Forecast {
    /// Constructs a new [Forecast].
    /// 
    /// The weather forecast to be retrieved is determinned based on execution time and `reference_time`,
    /// whether it is today's or tomorrow's forecast.
    /// If the execution time is before `reference_time`, it will retieve today's forecast;
    /// if it is `reference_time` or later, it will retrieve tomorrow's forecast.
    pub fn new(offices: String, area_code: String, reference_time: u32) -> Forecast {
        Forecast {
            offices: offices,
            area_code: area_code,
            area_name: String::new(),
            reference_time: reference_time,
            temp_lowest: None,
            temp_highest: None,
            update: Local::now(),
            forecast: None,
        }
    }

    /// gets forecast
    fn get_forecast(&mut self) {
        let api_url = format!("{}{}.json", API, self.offices);
        let client = match reqwest::blocking::Client::builder()
            .timeout(std::time::Duration::from_secs(60))
            .build()
        {
            Ok(c) => c,
            Err(why) => {
                error!("can't create client: {:?}", why);
                return;
            },
        };
        let res = match client.get(api_url).send() {
            Ok(r) => r,
            Err(why) => {
                error!("can't get http response: {:?}", why);
                return;
            },
        };
        let body = match res.text() {
            Ok(b) => b,
            Err(why) => {
                error!("can't get text from response: {:?}", why);
                return;
            },
        };
        match serde_json::from_str(&body) {
            Ok(j) => self.forecast = Some(j),
            Err(why) => {
                error!("can't get json from response: {:?}", why);
                return;
            },
        };
    }

    /// Gets the report datetime of a weather forecast.
    /// The path in forecast JSON for this is 0/reportDatetime.
    pub fn get_report_datetime(&mut self) -> Option<DateTime<Local>> {
        let json = match &self.forecast {
            Some(f) => f,
            _ => {
                warn!("no forecast");
                return None;
            }
        };
        let report_datetime_str = match json[0]["reportDatetime"].as_str() {
            Some(r) => r,
            _ => {
                warn!("can't get report datetime");
                return None;
            }
        };
        match DateTime::parse_from_rfc3339(report_datetime_str) {
            Ok(dt) => Some(dt.into()),
            Err(why) => {
                warn!("can't parse reportDatetime: {}", why);
                return None;
            }
        }
    }

    /// Gets a forecast and store the lowest, highest temperature, area name, update datetime
    /// and the forecast
    /// to [Forecast::temp_lowest], [Forecast::temp_highest], [Forecast::area_name],
    /// [Forecast::update] and [Forecast::forecast].
    pub fn update_temperature(&mut self) {
        self.get_forecast();
        let json = match &self.forecast {
            Some(j) => j,
            _ => {
                warn!("No forecast");
                return;
            },
        };
        let areas = json[0]["timeSeries"][2]["areas"].as_array().unwrap();
        let mut temps_json: Option<&Vec<serde_json::Value>> = None;
        for area in areas {
            if area["area"]["code"] == self.area_code {
                self.area_name = area["area"]["name"].as_str().unwrap().to_string();
                temps_json = area["temps"].as_array();
                break;
            }
        }
        let temps:Vec<f32> = match temps_json {
            Some(t) => t.iter().map(|x| x.as_str().unwrap().parse::<f32>().unwrap()).collect(),
            _ => return,
        };
        let time_defines = json[0]["timeSeries"][2]["timeDefines"].as_array().unwrap();

        let now = Local::now();
        let mut target_datetime = now
            .with_hour(0)
            .unwrap()
            .with_minute(0)
            .unwrap()
            .with_second(0)
            .unwrap()
            .with_nanosecond(0)
            .unwrap();
        if now.hour() >= self.reference_time {
            target_datetime += Duration::days(1);
        }

        let mut next_temps: Vec<f32> = Vec::new();
        for (time_define, temp) in time_defines.iter().zip(temps.iter()) {
            let time: DateTime<Local> = DateTime::parse_from_rfc3339(time_define.as_str().unwrap())
                .unwrap()
                .into();
            if time >= target_datetime && time < (target_datetime + Duration::days(1)) {
                next_temps.push(*temp);
            }
        }
        if next_temps.len() != 2 {
            warn!("mismatch number of temperatures");
            return;
        }
        self.temp_lowest = Some(next_temps[0]);
        self.temp_highest = Some(next_temps[1]);
        self.update = Local::now();
    }
}
