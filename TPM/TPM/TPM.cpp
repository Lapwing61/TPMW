// TPM.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
using namespace std;	
namespace bpt = boost::property_tree;

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
	string city_id;
	string lat;
	string lon;
	string zoom1;
	string zoom2;
};

typedef vector<city> vec_type;

struct weather_hourly
{
	string	utime;
	int		hour;
	string  icon;
	string	temperature;
};

typedef vector<weather_hourly> vec_wh;

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
	bindVariable(it, end, res.city_id); ++it;
	bindVariable(it, end, res.lat); ++it;
	bindVariable(it, end, res.lon); ++it;
	bindVariable(it, end, res.zoom1); ++it;
	bindVariable(it, end, res.zoom2); ++it;
}

int main()
{
	try{
			SetConsoleOutputCP(1251);

			bpt::ptree pt;
			bpt::ini_parser::read_ini("config.ini", pt);

			string provider_name = pt.get<string>("Provider.name");
			string provider_id = pt.get<string>("Provider.id");
			string secret_key = pt.get<string>("Provider.key");
			string output_type = pt.get<string>("Output.type");
			string output_dir = pt.get<string>("Output.dir");
			string output_file = pt.get<string>("Output.file");
			string output = output_dir + "\\" + output_file;

			CURL *curl;
			CURLcode res;
			curl_global_init(CURL_GLOBAL_DEFAULT);

			bpt::ptree pt3;
			bpt::ptree trf;
			bpt::ptree replacelayers;
			bpt::ptree objs;

			replacelayers.add("layer", "Погода на карте");
			trf.add_child("replacelayers", replacelayers);

			ifstream ifile("city.csv");
			if (!ifile.is_open())
			{
				throw runtime_error(" failed to open file city.csv");
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
				bpt::ptree obj;
				bpt::ptree sdata1;
				bpt::ptree sdata2;

				curl = curl_easy_init();

		//   		    if (((i % 10) == 0) && (i != 0)) Sleep(60000);

				string surl = "https://" + provider_name + "/forecast/" + secret_key + "/" + it->lat + "," + it->lon + "?units=si";
				char *url = new char[surl.length() + 1];
				strcpy(url, surl.c_str());
				string readBuffer;

				if (curl) {


					curl_easy_setopt(curl, CURLOPT_URL, url);
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

					res = curl_easy_perform(curl);	

					if (res != CURLE_OK) {
						throw runtime_error(" failed to connect");
					}
				}
				else return 1;

				curl_easy_cleanup(curl);

				boost::property_tree::ptree pt2;
				stringstream ss(readBuffer);
				boost::property_tree::json_parser::read_json(ss, pt2);

				weather_hourly current_weather;
				current_weather.utime = pt2.get<string>("currently.time");
				current_weather.hour = (((stoi(current_weather.utime)) % 86400) / 3600 + 8) % 24;

				current_weather.icon = pt2.get<string>("currently.icon");
				current_weather.temperature = pt2.get<string>("currently.temperature");
			
				vec_wh hourly_weather;

				BOOST_FOREACH(auto &v, pt2.get_child("hourly.data"))
				{
					weather_hourly tmp2;
					assert(v.first.empty()); 
					tmp2.utime = v.second.get<string>("time");
					tmp2.hour = (((stoi(tmp2.utime)) % 86400) / 3600 + 8) % 24;
					tmp2.icon = v.second.get<string>("icon");
					tmp2.temperature = v.second.get<string>("temperature");
					hourly_weather.push_back(tmp2);

				}

				string current_w;
				string first_w;
				string second_w;

				current_w = "%0D%0AСейчас: %3Cimg src=%22[ICON1]%22 /%3E " + current_weather.temperature + "°";

					if ((current_weather.hour >= 0) && (current_weather.hour <= 11)) {
						first_w  = "%0D%0AДнем: %3Cimg src=%22[ICON2]%22 /%3E " + hourly_weather[15 - (current_weather.hour % 12)].temperature + "°";
						second_w = "%0D%0AНочью: %3Cimg src=%22[ICON3]%22 /%3E " + hourly_weather[27 - (current_weather.hour % 12)].temperature + "°";
					}
					else {
						first_w  = "%0D%0AНочью: %3Cimg src=%22[ICON2]%22 /%3E " + hourly_weather[15 - (current_weather.hour % 12)].temperature + "°";
						second_w = "%0D%0AЗавтра: %3Cimg src=%22[ICON3]%22 /%3E " + hourly_weather[27 - (current_weather.hour % 12)].temperature + "°";
					}

				i++;
				sdata2.put("<xmlattr>.name", "#Name");
				sdata2.put_value(it->city_name);
				sdata1.add_child("sdata", sdata2);

				string comments;
				comments = it->city_name + current_w + first_w + second_w;
				sdata2.put("<xmlattr>.name", "#Comments");
				sdata2.put_value(comments);
				sdata1.add_child("sdata", sdata2);

				string labeldisp;
				labeldisp = it->city_name
					+ "%0D%0A" + "+23" + "°";
				sdata2.put("<xmlattr>.name", "#LabelDisp");
				sdata2.put_value(labeldisp);
				sdata1.add_child("sdata", sdata2);

				sdata2.put("<xmlattr>.name", "#HitZoneR");
				sdata2.put_value("0");
				sdata1.add_child("sdata", sdata2);

				time_t current_time = time(NULL);
				float delphi_time = ((float)current_time / 86400) + 25569;


					sdata2.put("<xmlattr>.name", "#CreateTime");
				sdata2.put_value(to_string(delphi_time));
				sdata1.add_child("sdata", sdata2);

				sdata2.put("<xmlattr>.name", "#CreateUser");
				sdata2.put_value("2388627466|Модуль импорта погоды");
				sdata1.add_child("sdata", sdata2);

				sdata2.put("<xmlattr>.name", "#ChangeTime");
				sdata2.put_value(to_string(delphi_time));
				sdata1.add_child("sdata", sdata2);

				sdata2.put("<xmlattr>.name", "#ChangeUser");
				sdata2.put_value("2388627466|Модуль импорта погоды");
				sdata1.add_child("sdata", sdata2);

				obj.add_child("sdata", sdata1);
				obj.put("<xmlattr>.id", it->city_id);
				obj.put("<xmlattr>.layer", "Погода на карте");
				obj.put("<xmlattr>.x", it->lon);
				obj.put("<xmlattr>.y", it->lat);
				obj.put("<xmlattr>.zoom1", it->zoom1);
				obj.put("<xmlattr>.zoom2", it->zoom2);
				objs.add_child("obj", obj);
			}
			trf.add_child("objs", objs);
			pt3.add_child("trf", trf);

			ofstream ofile(output);

			bpt::write_xml(ofile, pt3);
			curl_global_cleanup();
			return 0;
		}
		catch (runtime_error& e) {
			cerr << e.what();
			return 0;
		}
	}

