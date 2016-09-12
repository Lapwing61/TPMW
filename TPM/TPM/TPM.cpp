// TPM.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
using namespace std;

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

int main()
{

	char x;

	boost::property_tree::ptree pt;
	boost::property_tree::ini_parser::read_ini("config.ini", pt);

	string provider_name = pt.get<string>("Provider.name");
//	cout << pt.get<string>("Provider.id") << endl;
	string secret_key = pt.get<string>("Provider.key");
//	cout << pt.get<string>("Output.type") << endl;
//	cout << pt.get<string>("Output.dir") << endl;

	CURL *curl;
	CURLcode res;

	char errbuf[CURL_ERROR_SIZE];
	string readBuffer;

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();

	if (curl) {

	 	string surl = "https://" + provider_name + "/forecast/" + secret_key + "/52.2786307867,104.2666916628?units=si";
		char *url = new char[surl.length() + 1];
		strcpy(url, surl.c_str());
		errbuf[0] = 0;

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);	
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

		res = curl_easy_perform(curl);	

		if (res != CURLE_OK) {
			size_t len = strlen(errbuf);
			fprintf(stderr, "\nlibcurl: (%d) ", res);
			if (len)
				fprintf(stderr, "%s%s", errbuf,
				((errbuf[len - 1] != '\n') ? "\n" : ""));
			else
				fprintf(stderr, "%s\n", curl_easy_strerror(res));
		}
		else
			cout << readBuffer << endl;

		curl_easy_cleanup(curl);

	}
		
	cin >> x;

	curl_global_cleanup();
	return 0;

}

