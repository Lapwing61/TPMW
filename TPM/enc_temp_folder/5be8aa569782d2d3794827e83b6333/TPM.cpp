// TPM.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
using namespace std;	

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;	
}

typedef boost::char_separator<char> sep_type;
typedef boost::tokenizer<sep_type> tok_type;

struct my_type
{
	int val;
};

ostream & operator << (ostream & strm, const my_type & t)
{
	strm << t.val;
	return strm;
}

istream & operator >> (istream & strm, my_type & t)
{
	strm >> t.val;
	return strm;
}

struct city
{
	string city_name;
	string lat;
	string lon;
};

struct weather_hourly
{
	string	utime;
	int		hour;
	string  icon;
	string	temperature;
};

typedef vector<city> vec_type;

template <typename InputIterator, typename ValueT>
void bindVariable(InputIterator pos, InputIterator end, ValueT & val)
{
	if (pos == end)
	{
		throw runtime_error("bad csv format");
	}
	else
	    val = boost::lexical_cast<ValueT>(*pos);
}


template <typename InputIterator>
void parseCsvLine(InputIterator it, InputIterator end, city & res)
{
	bindVariable(it, end, res.city_name); ++it;
	bindVariable(it, end, res.lat); ++it;
	bindVariable(it, end, res.lon); ++it;
}

int main()
{
	SetConsoleOutputCP(1251);

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
	curl_global_init(CURL_GLOBAL_DEFAULT);

	char errbuf[CURL_ERROR_SIZE];

	ifstream ifile("city.csv");
	if (!ifile.is_open())
	{
		throw runtime_error("faild to open file city.csv");
	}

	string line;
	sep_type sep(";");
	vec_type vec;

	while (!ifile.eof())
	{
		getline(ifile, line);
		tok_type tok(line, sep);

		city tmp;
		parseCsvLine(tok.begin(), tok.end(), tmp);
		vec.push_back(tmp);
	}

	int i = 0;

	for (vec_type::const_iterator it = vec.begin(); it != vec.end(); ++it)
	{
			curl = curl_easy_init();

//   		    if (((i % 10) == 0) && (i != 0)) Sleep(60000);

			cout <<
				"Number: " << i <<
				" City: " << it->city_name <<
				" Lat: " << it->lat <<
				" Lon: " << it->lat << endl;

			if (curl) {

	 			string surl = "https://" + provider_name + "/forecast/" + secret_key + "/" + it->lat + "," + it->lon + "?units=si";
				char *url = new char[surl.length() + 1];
				strcpy(url, surl.c_str());
				errbuf[0] = 0;
				string readBuffer;

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
//				else
//					cout << readBuffer << endl;	

				boost::property_tree::ptree pt2;
				stringstream ss(readBuffer);
				boost::property_tree::json_parser::read_json(ss, pt2);

				weather_hourly current_weather;

				current_weather.utime = pt2.get<string>("currently.time");
				current_weather.hour = (((stoi(current_weather.utime)) % 86400) / 3600 + 8) % 24;

				current_weather.icon = pt2.get<string>("currently.icon");
				current_weather.temperature = pt2.get<string>("currently.temperature");


				cout <<
//					" Time: " << current_weather.utime <<
					" Hour: " << current_weather.hour <<
					" Icon: " << current_weather.icon <<
					" Temperature: " << current_weather.temperature << endl;

				weather_hourly hourly_weather[50];
				int j = 0;

				BOOST_FOREACH(auto &v, pt2.get_child("hourly.data"))
				{

					assert(v.first.empty()); 
					hourly_weather[j].utime = v.second.get<string>("time");
					hourly_weather[j].hour = (((stoi(hourly_weather[j].utime)) % 86400) / 3600 + 8) % 24;

					hourly_weather[j].icon = v.second.get<string>("icon");
					hourly_weather[j].temperature = v.second.get<string>("temperature");

					cout <<
//						"      Time: " << hourly_weather[j].utime <<
						"      Hour: " << hourly_weather[j].hour <<
						"      Icon: " << hourly_weather[j].icon <<
						"      Temperature: " << hourly_weather[j].temperature << endl;

					j++;

				}

				curl_easy_cleanup(curl);

			} 
			 i++;

	}

	cin >> x;

	curl_global_cleanup();
	return 0;

}

