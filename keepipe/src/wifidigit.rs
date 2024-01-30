//! Send a command to WiFiDigit over HTTP.

use reqwest;

#[derive(Debug)]
pub enum WiFiDigit {
    GET(String),
    PUT(String),
}

impl WiFiDigit {
    /// Constructs a new [WiFiDigit].
    /// `method` is 'GET' or 'PUT'.
    pub fn new(method: String, url: String) -> Result<WiFiDigit, String> {
        match method.as_str() {
            "get" => Ok(WiFiDigit::GET(url)),
            "GET" => Ok(WiFiDigit::GET(url)),
            "put" => Ok(WiFiDigit::PUT(url)),
            "PUT" => Ok(WiFiDigit::PUT(url)),
            _ => Err(String::from("unknown method")),
        }
    }

    /// Makes the GET or PUT request from `command` and send it.
    /// The request composed of url from [`WiFiDigit::new`] and `command`.
    /// 
    /// For example, if the `url` is 'http://192.168.0.1/' and the `command` is 'blink/1000.10', the request is:
    /// ```sh
    ///  http://192.168.0.1/blink/1000.10
    /// ```
    pub fn send(self, command: String) -> String {
        let client = reqwest::blocking::Client::builder()
            .timeout(std::time::Duration::from_secs(60))
            .build()
            .unwrap();
        match self {
            WiFiDigit::GET(url) => client
                .get(format!("{}/{}", url, command))
                .send()
                .unwrap()
                .text()
                .unwrap(),
            WiFiDigit::PUT(url) => client
                .put(format!("{}/{}", url, command))
                .send()
                .unwrap()
                .text()
                .unwrap(),
        }
    }
}