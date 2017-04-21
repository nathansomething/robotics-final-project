#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <curl/curl.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using std::array;
using std::string;
using std::shared_ptr;
using std::runtime_error;
using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;

using rapidjson::Document;
using rapidjson::Value;
using rapidjson::StringBuffer;
using rapidjson::Writer;
using rapidjson::kObjectType;

const bool debug = true;

// Execute a command from the command line and return result as a string
string exec(const char* cmd) {
    array<char, 128> buffer;
    string result;
    shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != NULL)
            result += buffer.data();
    }
    return result;
}

// Convert an FLAC file (specified by direction) to a base64 string
string getBase64(string direction) {
  exec(("./base64 -e " + direction + ".flac base64_output.txt").c_str());
  ifstream t("base64_output.txt");
  string output((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());
  return output;
}

//Helper function for curl request
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

/*
// Parse the direction from the JSON Request
string getDirection(string jsonRequest) {

  Document d;
  StringBuffer sb;

  d.Parse(jsonRequest);
  PrettyWriter<StringBuffer> writer(sb);
  document.Accept(writer);    // Accept() traverses the DOM and generates Handler events.
  return sb.GetString();
}
*/

// Returns a JSON object coresponding to a Google Search API Request
void getGoogleSpeechApiRequest(string direction, string recording) {
  // Attempting to use rapidJSON, but facing a newline encoding issue with the
  // content field

  // Document d;
  // d.SetObject();
  //
  // Document::AllocatorType& allocator = d.GetAllocator();
  //
  // size_t sz = allocator.Size();
  //
  // Value configs(kObjectType);
  // configs.AddMember("encoding", "FLAC", allocator);
  // configs.AddMember("sampleRateHertz", "16000", allocator);
  // configs.AddMember("languageCode", "en-US", allocator);
  //
  // Value audio(kObjectType);
  // Value recording_val(recording.c_str(), allocator);
  // audio.AddMember("content", recording_val, allocator);
  //
  // // Sample Audio for testing
  // // audio.AddMember("uri", "gs://cloud-samples-tests/speech/brooklyn.flac", allocator);
  //
  // d.AddMember("config", configs, allocator);
  // d.AddMember("audio", audio, allocator);
  //
  // // Convert JSON document to string
  // StringBuffer strbuf;
  // Writer<StringBuffer> writer(strbuf);
  // d.Accept(writer);

  string json_request = "{\"config\":{\"encoding\":\"FLAC\",\"sampleRateHertz\":16000,\"languageCode\":\"en-US\"},\"audio\":{\"content\":\"" + recording + "\"}}";
  ofstream my_request;
  my_request.open("my-request.json");
  my_request << json_request;
  my_request.close();
}

// Send a curl request to the google speech api
string sendGoogleSpeechRequest(string token) {
  // CURL *curl;
  // CURLcode res;
  // string output;
  //
  // curl = curl_easy_init();
  // if(curl) {
  //   struct curl_slist *headers = NULL;
  //   headers = curl_slist_append(headers, "Content-Type: application/json");
  //   headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());
  //
  //   curl_easy_setopt(curl, CURLOPT_URL, "https://speech.googleapis.com/v1/speech:recognize");
  //   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  //   curl_easy_setopt(curl, CURLOPT_POST, 1);
  //   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.c_str());
  //   curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request.length());
  //   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);  // for --insecure option
  //   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  //   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
  //   res = curl_easy_perform(curl);
  //   curl_easy_cleanup(curl);
  //   cout << "CURL" << endl << curl << endl;
  // }
  // return output;
  cout << "Before CURL" << endl;
  string curl_str = "curl -s -k -H 'Content-Type: application/json' -H 'Authorization: Bearer " + token + "' https://speech.googleapis.com/v1/speech:recognize -d @my-request.json";
  cout << "CURL String: " << curl_str << endl;
  string response = exec(curl_str.c_str());
  return response;
}

// Custom debug function that can easily be disabled
void debug_str(string text) {
  if (debug) {
    cout << text << endl;
  }
}

int main(int argc, char *argv[]) {

  // Error Handeling
  if(argc < 2) {
    cout << "Direction Not specified" << endl;
    return 1;
  }
  const string direction = argv[1];
  if(direction != "up" &&
     direction != "down" &&
     direction != "left" &&
     direction != "right") {
       cout << "Invalid Direction" << endl;
       return 1;
  }

  // Get the path of the private key (necessary for interfacing with GoogleSpeechAPI)
  string pwd = exec("pwd");
  // Erase the last character (\n)
  const string path = string(pwd).erase(pwd.length() - 1, pwd.length() -2);
  // Calculate the private key
  const string private_key = path + "/Robotics-7f0a2eeb2573.json";
  debug_str(private_key);

  // Authenticate the service account
  exec(("gcloud auth activate-service-account --key-file=" + private_key).c_str());

  // Set Google Application Credentials to point to the private key for the Google Speech API
  // Get access token
  const string token_temp = exec(("export GOOGLE_APPLICATION_CREDENTIALS=" + private_key + "&& gcloud auth application-default print-access-token").c_str());
  const string token = string(token_temp).erase(token_temp.length() - 1, token_temp.length() -2);
  debug_str(token);

  const string recording = getBase64(direction);

  // Format the JSON request to send to the Google Speech API
  getGoogleSpeechApiRequest(direction, recording);

  // Send a CURL Request to the Google Speech API and get the result
  const string response = sendGoogleSpeechRequest(token);
  debug_str(("Response: " + response).c_str());

  return 0;
}
