#include <iostream>
#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <nlohmann/json.hpp>
#include "libs/Weather.h"
#include <curl/curl.h>

using json = nlohmann::json;

void send_json(SSL* ssl, json data);
int main() {
    // Inicjalizacja OpenSSL
    SSL_library_init();
    SSL_load_error_strings();



    SSL_load_error_strings();

    // Tworzenie kontekstu SSL
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());

  // Sprawdzanie poprawności certyfikatu
    int attempts = 0;
    while(attempts < 3) {
        if (SSL_CTX_load_verify_locations(ctx, "server.crt", NULL) != 1) 
        {
            std::cerr << "Nie można załadować certyfikatu!" << std::endl;
            attempts++;
        }
        else
        {
            std::cout << "Certyfikat załadowany poprawnie" << std::endl;
            SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM);
            break;
        }
    }
    if (attempts == 3) {
        std::cerr << "Nie udało się załadować certyfikatu po 3 próbach, kończymy program" << std::endl;
        return 1;
    }

    // Ustawianie klucza prywatnego
    SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM);


    // Sprawdzanie poprawności klucza prywatnego
    if (!SSL_CTX_check_private_key(ctx)) {
        std::cerr << "Klucz prywatny nie pasuje do certyfikatu publicznego!" << std::endl;
        abort();
    }
    else
    {
        std::cout << "Klucz prywatny pasuje do certyfikatu publicznego" << std::endl;
        
    }


    // Tworzenie gniazda i przygotowywanie do nasłuchiwania
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(2137);

  // sprawdzenie czy port jest otwarty
    if (bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Nie można otworzyć portu!" << std::endl;
        abort();
    }
    
    
    //sparwdzenie czy port jest otwarty
    if (listen(sockfd, 5) < 0) {
        std::cerr << "Nie można otworzyć portu!" << std::endl;
        abort();
    }
   
    printf("Waiting for connections...\n");

          


    // Pętla obsługująca połączenia
    while (true) {
        sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int client = accept(sockfd, (sockaddr*)&cli_addr, &cli_len);

        // Tworzenie obiektu SSL
        SSL* ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client);

        // Akceptacja połączenia i przeprowadzanie komunikacji
        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        } else 
        {
            // Odczytanie wiadomości od klienta
            char buf[1024] = {0};
            int bytes = SSL_read(ssl, buf, sizeof(buf));
            std::string message(buf, bytes);
            json recvJson = json::parse(message);
            std::string location = recvJson["city"];;
            std::cout << "Otrzymano wiadomość: " << location << std::endl;
            
           // pobranie i przesyłanie pogody
            
            Weather weather;
                  
            weather.setLocation(location);
            weather.setResponseWeather();
            weather.parseServerResponse();
            send_json(ssl, weather.getResponseWeather());
            std::cout << std::fixed <<  std::setprecision(2) << weather.parseServerResponse() << std::endl;
            std::cout << "Wysłano pogodę" << std::endl;
           
        }

        // Zamykanie połączenia i zwalnianie zasobów
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client);
    }

    // Zwalnianie zasobów kontekstu SSL
    SSL_CTX_free(ctx);

    return 0;
}

// funkcja wysyłająca jsona do klienta przez ssl
void send_json(SSL* ssl, json data) {
    // konwertowanie jsona na string
    std::string json_str = data.dump();
    // pobranie długości stringa
    int json_len = json_str.length();
    // wysłanie 
    SSL_write(ssl, json_str.c_str(), json_len);
}