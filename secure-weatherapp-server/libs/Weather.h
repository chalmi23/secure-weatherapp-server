#include <iostream>
#include <ctime>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#define MAX_ATTEMPTS 2

using json = nlohmann::json;

using namespace std;

class Weather {
    public:
        json getWeather(std::string location);
        
        // setter i getter do responseWeather
        std::string getParsedWeather()  { return this->parseServerResponse(); }

        void setResponseWeather() { this->weatherJSON = getWeather(this->location); }
        void setReceivedWeather(json responseWeather) { this->weatherJSON = responseWeather; }
        json getResponseWeather() { return weatherJSON; }
        
       
        // setter i getter do location
        std::string getLocation() const { return location; }
        void setLocation(std::string location) { this->location = location; }
        std::string parseServerResponse();

        
    private:
        static std::size_t writeCallback(char* ptr, int size, int nmemb, void* userdata);
        std::string location;
        json weatherJSON;
};

std::size_t Weather::writeCallback(char* ptr, int size, int nmemb, void* userdata) {
    std::string* stream = (std::string*)userdata;
    int realsize = size * nmemb;
    stream->append(ptr, realsize);
    return realsize;
}

json Weather::getWeather(std::string location) {
    CURL* curl;
    CURLcode res;
    std::string stream;
    int attempts = 0;
   
    long http_code = 0;
    do {
        curl = curl_easy_init();
        if (curl) {
            std::string url = "https://api.openweathermap.org/data/2.5/weather?q=" + location + "&appid=01bfc1473b89420ac08c560a25c1b535";
            std:: cout << url << std::endl;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stream);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);

            res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                std::cout << "CURL error: " << curl_easy_strerror(res) << std::endl;
                weatherJSON = "Error";
            } 
            else 
            {
                // get the http response code
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
                if(http_code == 200)
                {
                    try
                    {
                        if(stream.empty())
                        {
                            weatherJSON = "Error";
                             return weatherJSON;
                        }
                        else
                        {
                            weatherJSON = json::parse(stream);
                            weatherJSON["status"]= "ok";
                            std:: cout << "parseServerResponse" << std::endl;
                           
                        }
                    ;
                    }
                    catch (json::parse_error& e) 
                    {
                        std::cout << "Error while parsing JSON: " << e.what() << std::endl;

                        weatherJSON["status"]= "error";
                        return weatherJSON;
                        
                    }                    
                    
                }
                else
                {
                    std::cout << "Error, response code: " << http_code << std::endl;
                    weatherJSON["status"]= "error";
                     return weatherJSON;
                }
            }
            curl_easy_cleanup(curl);
            }
            attempts++;
            } while (http_code != 200 && attempts < MAX_ATTEMPTS);
            return weatherJSON;
            }


// getServerResponse

 std::string Weather::parseServerResponse()
{
    if (weatherJSON["status"] == "ok")
    {
    
        std::string weatherDescription = weatherJSON["weather"][0]["description"].get<std::string>();
        double temperature = weatherJSON["main"]["temp"].get<double>() - 273.15;
        double pressure = weatherJSON["main"]["pressure"].get<double>();
        double windSpeed = weatherJSON["wind"]["speed"].get<double>();
        double humidity = weatherJSON["main"]["humidity"].get<double>();
        std::string city = weatherJSON["name"].get<std::string>();

       // string response with weather data
       std::string response = "Weather in " + city + " is " + weatherDescription + ".\nTemperature is " + 
       std::to_string(temperature) + " degrees Celsius.\nPressure is " + std::to_string(pressure) + " hPa.\nWind speed is " + 
       std::to_string(windSpeed) + " m/s.\nHumidity is " + std::to_string(humidity) + "%.";

       
        //std:: cout << "parseServerResponse" << std::endl;
        //std:: cout<< response << endl;

    return response; 
    }
    else
    {
        return "Error";
    }

      

  
    
   

}