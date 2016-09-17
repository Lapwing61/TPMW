// TPM.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
using namespace std;	
namespace bpt = boost::property_tree;
namespace bpo = boost::program_options;

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
		throw runtime_error("city.csv: bad csv format");
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

int main(int ac, char* av[])
{
	try{
			SetConsoleOutputCP(1251);

			bpo::options_description desc("Allowed options");
			desc.add_options()
				("help,h", "help message")
				("config,c", bpo::value<std::string>(), "configuration file")
				("list,l", bpo::value<std::string>(), "list of points")
				("key,k", bpo::value<std::string>(), "secret key")
				("output,o", bpo::value<std::string>(), "outup XML file")
				;

			bpo::variables_map vm;
			bpo::parsed_options parsed = bpo::command_line_parser(ac, av).options(desc).allow_unregistered().run();
			bpo::store(parsed, vm);
			bpo::notify(vm);

			if (vm.count("help")) {
				cout << desc << "\n";
				return 0;
			}

			string list;
			if (vm.count("list")) {
				list = vm["list"].as<string>();
				if (list.empty()) list = "city.csv";
			}
			else list = "city.csv";

			string provider_name;
			string provider_id;
			string secret_key;
			string output_type;
			string output_dir;
			string output_file;
			string output;
			string clear_day;
			string clear_night;
			string rain;
			string snow;
			string sleet;
			string wind;
			string fog;
			string cloudy;
			string partly_cloudy_day;
			string partly_cloudy_night;


			if (ac == 1) {
				bpt::ptree pt;
				bpt::ini_parser::read_ini("config.ini", pt);
				provider_name = pt.get<string>("Provider.name");
//				provider_id = pt.get<string>("Provider.id");
				secret_key = pt.get<string>("Provider.key");
//				output_type = pt.get<string>("Output.type");
				output_dir = pt.get<string>("Output.dir");
				output_file = pt.get<string>("Output.file");
				output = output_dir + "\\" + output_file;
				clear_day = pt.get<string>("Icons.clear_day");
				clear_night = pt.get<string>("Icons.clear_night");
				rain = pt.get<string>("Icons.rain");
				snow = pt.get<string>("Icons.snow");
				sleet = pt.get<string>("Icons.sleet");
				wind = pt.get<string>("Icons.wind");
				fog = pt.get<string>("Icons.fog");
				cloudy = pt.get<string>("Icons.cloudy");
				partly_cloudy_day = pt.get<string>("Icons.partly_cloudy_day");
				partly_cloudy_night = pt.get<string>("Icons.partly_cloudy_night");

			}
			else 
				if (vm.count("config")) {
					string config = vm["config"].as<string>();
					if (config.empty()) config = "config.ini";
					bpt::ptree pt;
					bpt::ini_parser::read_ini((string)config, pt);
					provider_name = pt.get<string>("Provider.name");
//					provider_id = pt.get<string>("Provider.id");
					secret_key = pt.get<string>("Provider.key");
//					output_type = pt.get<string>("Output.type");
					output_dir = pt.get<string>("Output.dir");
					output_file = pt.get<string>("Output.file");
					output = output_dir + "\\" + output_file;
					clear_day = pt.get<string>("Icons.clear_day");
					clear_night = pt.get<string>("Icons.clear_night");
					rain = pt.get<string>("Icons.rain");
					snow = pt.get<string>("Icons.snow");
					sleet = pt.get<string>("Icons.sleet");
					wind = pt.get<string>("Icons.wind");
					fog = pt.get<string>("Icons.fog");
					cloudy = pt.get<string>("Icons.cloudy");
					partly_cloudy_day = pt.get<string>("Icons.partly_cloudy_day");
					partly_cloudy_night = pt.get<string>("Icons.partly_cloudy_night");
				}
				else {
					provider_name = "api.forecast.io";
					if (!vm.count("key")) {
						throw runtime_error("secret key needs");
					}
					else secret_key = vm["key"].as<string>();
					if (!vm.count("output")) {
						output = "testXml.xml";
					}
					else {
						output = vm["output"].as<string>();
						if (output.empty()) output = "testXml.xml";
						}
					clear_day = "_CLEAR__";
					clear_night = "CLEARNT";
					rain = "__RAIN__";	 
					snow = "__SNOW__";	 
					sleet = "_SLEET__";	 
					wind = "__WIND__";	 
					fog = "__FOG___";	 
					cloudy = "_CLOUDY_"; 
					partly_cloudy_day = "_PTCLDY_";
					partly_cloudy_night = "_PTCLNT_";
				}

			CURL *curl;
			CURLcode res;
			curl_global_init(CURL_GLOBAL_DEFAULT);

			bpt::ptree pt3;
			bpt::ptree trf;
			bpt::ptree replacelayers;
			bpt::ptree objs;

			replacelayers.add("layer", "Погода на карте");
			trf.add_child("replacelayers", replacelayers);

			ifstream ifile(list);
			if (!ifile.is_open())
			{
				throw runtime_error(list + ": cannot open file"); 
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
				bpt::ptree signs;
				bpt::ptree sign;

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

				int tz = 0;
				if (pt2.get<string>("timezone") == "Asia/Yekaterinburg")  tz = 5;
				if (pt2.get<string>("timezone") == "Asia/Omsk")  tz = 6;
				if (pt2.get<string>("timezone") == "Asia/Krasnoyarsk")  tz = 7;
				if (pt2.get<string>("timezone") == "Asia/Irkutsk")  tz = 8;
				if (pt2.get<string>("timezone") == "Asia/Chita")  tz = 8;
				if (pt2.get<string>("timezone") == "Asia/Yakutsk")  tz = 9;

				weather_hourly current_weather;
				current_weather.utime = pt2.get<string>("currently.time");
				current_weather.hour = (((stoi(current_weather.utime)) % 86400) / 3600 + tz) % 24;

				string icon = pt2.get<string>("currently.icon");
				if (icon == "clear-day") current_weather.icon = clear_day;
				if (icon == "clear-night") current_weather.icon = clear_night;
				if (icon == "rain") current_weather.icon = rain;
				if (icon == "snow") current_weather.icon = snow;
				if (icon == "sleet") current_weather.icon = sleet;
				if (icon == "wind") current_weather.icon = wind;
				if (icon == "fog") current_weather.icon = fog;
				if (icon == "cloudy") current_weather.icon = cloudy;
				if (icon == "partly-cloudy-day") current_weather.icon = partly_cloudy_day;
				if (icon == "partly-cloudy-night") current_weather.icon = partly_cloudy_night;

				current_weather.temperature = to_string(lround(stoi(pt2.get<string>("currently.temperature"))));
			
				vec_wh hourly_weather;

				BOOST_FOREACH(auto &v, pt2.get_child("hourly.data"))
				{
					weather_hourly tmp2;
					assert(v.first.empty()); 
					tmp2.utime = v.second.get<string>("time");
					tmp2.hour = (((stoi(tmp2.utime)) % 86400) / 3600 + 8) % 24;
					string icon = v.second.get<string>("icon");
					if (icon == "clear-day") tmp2.icon = clear_day;
					if (icon == "clear-night") tmp2.icon = clear_night;
					if (icon == "rain") tmp2.icon = rain;
					if (icon == "snow") tmp2.icon = snow;
					if (icon == "sleet") tmp2.icon = sleet;
					if (icon == "wind") tmp2.icon = wind;
					if (icon == "fog") tmp2.icon = fog;
					if (icon == "cloudy") tmp2.icon = cloudy;
					if (icon == "partly-cloudy-day") tmp2.icon = partly_cloudy_day;
					if (icon == "partly-cloudy-night") tmp2.icon = partly_cloudy_night;
					tmp2.temperature = to_string(lround(stoi(v.second.get<string>("temperature"))));
					hourly_weather.push_back(tmp2);

				}

				string current_w;
				string first_w;
				string second_w;

				current_w = "%0D%0AСейчас: %3Cimg src=%22[" + current_weather.icon + "]%22 /%3E " + current_weather.temperature + "°";

					if ((current_weather.hour >= 0) && (current_weather.hour <= 11)) {
						first_w  = "%0D%0AДнем: %3Cimg src=%22[" + hourly_weather[15 - (current_weather.hour % 12)].icon + "]%22 /%3E " + hourly_weather[15 - (current_weather.hour % 12)].temperature + "°";
						second_w = "%0D%0AНочью: %3Cimg src=%22[" + hourly_weather[27 - (current_weather.hour % 12)].icon + "]%22 /%3E " + hourly_weather[27 - (current_weather.hour % 12)].temperature + "°";
					}
					else {
						first_w  = "%0D%0AНочью: %3Cimg src=%22[" + hourly_weather[15 - (current_weather.hour % 12)].icon + "]%22 /%3E " + hourly_weather[15 - (current_weather.hour % 12)].temperature + "°";
						second_w = "%0D%0AЗавтра: %3Cimg src=%22[" + hourly_weather[27 - (current_weather.hour % 12)].icon + "]%22 /%3E " + hourly_weather[27 - (current_weather.hour % 12)].temperature + "°";
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
				labeldisp = it->city_name + "%0D%0A" + current_weather.temperature + "°";
				sdata2.put("<xmlattr>.name", "#LabelDisp");
				sdata2.put_value(labeldisp);
				sdata1.add_child("sdata", sdata2);

				sdata2.put("<xmlattr>.name", "#HitZoneR");
				sdata2.put_value("0");
				sdata1.add_child("sdata", sdata2);

				time_t current_time = time(NULL);
				double delphi_time = ((double)current_time / 86400) + 25569;

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

				string libcrc = "%22[" + current_weather.icon + "]%22";
 				sign.put("<xmlattr>.x", 0);
				sign.put("<xmlattr>.y", 0);
				sign.put("<xmlattr>.libcrc", libcrc);
				signs.add_child("sign", sign);
				obj.add_child("signs", signs);

				double EarthMapSize = 134217728;
				double gx = stod(it->lon);
				double gy = stod(it->lat);
				double mx;
				double my;
				if (gx == 0) mx = 0; else {
					double z = sin( gx / 180 * M_PI );
					if (z > 1 - (1e-6)) z = 1 - (1e-6);
					if (z < -1 + (1e-6)) z = -1 + (1e-6);
					mx = 0.5 * log((1 + z) / (1 - z))* EarthMapSize / (2 * M_PI);
				}
				if (gy == 0) my = 0; else {
					my = gy * EarthMapSize / 360;
				}

				obj.put("<xmlattr>.id", it->city_id);
				obj.put("<xmlattr>.layer", "Погода на карте");
				obj.put("<xmlattr>.x", to_string(mx));
				obj.put("<xmlattr>.y", to_string(my));
				obj.put("<xmlattr>.zoom1", it->zoom1);
				obj.put("<xmlattr>.zoom2", it->zoom2);
				objs.add_child("obj", obj);
			}
			trf.add_child("objs", objs);
			pt3.add_child("trf", trf);

			ofstream ofile(output);

			auto settings = bpt::xml_writer_make_settings<string>('\t', 1, "windows-1252");
			bpt::write_xml(ofile, pt3, settings);
			curl_global_cleanup();
			return 0;
		}
		catch (runtime_error& e) {
			cerr << e.what();
				return 0;
		}
	}

