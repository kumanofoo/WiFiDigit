use chrono::{Local, Timelike};
use clap::Parser;
use keepipe::config::Config;
use keepipe::forecast::Forecast;
use keepipe::wifidigit;
use log::{debug, error, warn};
use std::path::PathBuf;

#[derive(Debug, Parser)]
struct Args {
    /// Check configuration TOML
    #[arg(short, long)]
    test_config: bool,
    #[arg(short, long, default_value = "./keepipe.toml")]
    config: PathBuf,
    #[arg(short, long)]
    verbose: bool,
}

fn main() {
    env_logger::init();
    let args = Args::parse();
    debug!("config TOML path: {:?}", args.config);
    if !args.config.exists() {
        error!("configuration file not found");
        return;
    }
    let config = Config::new(&args.config).unwrap();
    if args.test_config {
        return;
    }
    debug!("config: {:?}", config);

    let mut f = Forecast::new(
        config.area.jma_offices,
        config.area.jma_area_code,
        config.area.reference_time.unwrap_or(5),
    );
    f.update_temperature();

    let temp_lowest: f32 = match f.temp_lowest {
        Some(t) => t,
        _ => {
            warn!("no lowest temperature");
            return;
        }
    };
    let temp_highest: f32 = match f.temp_highest {
        Some(t) => t,
        _ => {
            warn!("no highest temperature");
            return;
        }
    };
    if args.verbose {
        println!("report datetime: {}", f.get_report_datetime().unwrap());
        println!("area: {}", f.area_name);
        println!("  lowest: {}", temp_lowest);
        println!("  highest: {}", temp_highest);
    }

    for action in config.actions.clone() {
        let mut command = action
            .command
            .replace("{tempLowest}", temp_lowest.to_string().as_str());
        command = command.replace("{tempHighest}", temp_highest.to_string().as_str());
        if command.contains("{reportHour}") {
            if let Some(dt) = f.get_report_datetime() {
                command = command.replace("{reportHour}", dt.hour().to_string().as_str());
            } else {
                warn!("skipped a action because can't get 'reportDate', dispite the command contains '{{reportHour}}'.");
                continue;
            }
        }
        if command.contains("{nowHour}") {
            let now = Local::now();
            let hour12 = now.hour12().1;
            command = command.replace("{nowHour}", hour12.to_string().as_str());
        }
        if let Some(lowest_threshold) = action.lowest {
            if temp_lowest > lowest_threshold {
                continue;
            }
        }
        if let Some(highest_threshold) = action.highest {
            if temp_highest < highest_threshold {
                continue;
            }
        }
        let method = wifidigit::WiFiDigit::new(
            action.method.unwrap_or(String::from("GET")),
            config.wifidigit.url.clone(),
        )
        .unwrap();
        debug!("command: {}", command);
        method.send(command);
    }
}
