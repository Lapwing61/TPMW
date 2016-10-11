// TPM.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
using namespace std;	
namespace bpt = boost::property_tree;
namespace bpo = boost::program_options;

static size_t WriteCallback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	((string*)stream)->append((char*)ptr, size * nmemb);
	return size * nmemb;	
}

//static size_t ReadCallback(void *ptr, size_t size, size_t nmemb, void *stream)
//{
//	((string*)ptr)->append((char*)stream, size * nmemb);
//	return size * nmemb;
//}

static size_t ReadCallback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t retcode;
	retcode = fread(ptr, size, nmemb, (FILE*)stream);
	return retcode;
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
	string  icon_src;
	string  icon_crc;
	string	temperature;
};

typedef vector<weather_hourly> vec_wh;

template <typename InputIterator, typename ValueT>
void bindVariable(InputIterator pos, InputIterator end, ValueT & val)
{
	if (pos == end)
	{
		throw runtime_error("city.csv: bad csv format, aborting");
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

string curr_date()
{
	stringstream tt;
	string dd;
	time_t t = time(nullptr);
	tm tm = *localtime(&t);
	tt << put_time(&tm, "%y%m%d");
 	dd = tt.str();
	return dd;
}

string curr_time()
{
	stringstream tt;
	string dd;
	time_t t = time(nullptr);
	tm tm = *localtime(&t);
	tt << put_time(&tm, "%T");
	dd = tt.str();
	return dd;
}

boost::filesystem::ofstream flog;

string getkey(bpt::ptree tree, string key)
{
	string okey;
	if (boost::optional<std::string> str = tree.get_optional<std::string>(key)) {
		okey = str.get();
	}
	else {
		throw runtime_error("config.ini: [" + key + "] not found, aborting");
	}
	if (okey == "") {
		throw runtime_error("config.ini: [" + key + "] empty, aborting");
	}
	return okey;
}

string getkeyd(bpt::ptree tree, string key, string def)
{
	string okey;
	if (boost::optional<std::string> str = tree.get_optional<std::string>(key)) {
		okey = str.get();
	}
	else {
		okey = def;
	}
	return okey;
}

int main(int ac, char* av[])
{
	SetConsoleOutputCP(1251);

	time_t seconds;
	stringstream sec;

	time(&seconds);
	sec << seconds;
	string spath = sec.str();
	string lpath = "\\LOG\\" + curr_date();
	string lfile = "\\TPM_" + spath + ".log";
	boost::filesystem::path cpath = boost::filesystem::current_path();
	boost::filesystem::path logpath;
	logpath += cpath;
	logpath += lpath;
	if (!boost::filesystem::is_directory(logpath)) boost::filesystem::create_directories(logpath);
	boost::filesystem::path logfile;
	logfile = logpath;
	logfile += lfile;
	boost::filesystem::ofstream flog(logfile);

	try{

		flog << curr_time() << ": Starting" << endl;

		bpo::options_description desc("Allowed options");
			desc.add_options()
				("help,h", "help message")
				("config,c", bpo::value<std::string>(), "configuration file")
				("list,l", bpo::value<std::string>(), "list of points")
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
			string output_site;
			string output_login;
			string output_password;
			string output_file;
			string output = "";

			string src_clear_day;
			string src_clear_night;
			string src_rain;
			string src_rain_2;
			string src_snow;
			string src_snow_2;
			string src_sleet;
			string src_wind;
			string src_fog;
			string src_cloudy;
			string src_partly_cloudy_day;
			string src_partly_cloudy_night;

			string crc_clear_day;
			string crc_clear_night;
			string crc_rain;
			string crc_rain_2;
			string crc_snow;
			string crc_snow_2;
			string crc_sleet;
			string crc_wind;
			string crc_fog;
			string crc_cloudy;
			string crc_partly_cloudy_day;
			string crc_partly_cloudy_night;

			int out = 0;

			if (ac == 1) {
				bpt::ptree pt;
				bpt::ini_parser::read_ini("config.ini", pt);
				provider_name = getkey(pt, "Provider.name");
//				provider_id = getkey(pt, "Provider.id");
				secret_key = getkey(pt, "Provider.key");

//				output_type = getkeyd(pt, "Output.type","XML");
				output_file = getkeyd(pt, "Output.file", "Weather.xmltrf");
				output_dir = getkeyd(pt,"Output.dir","");
				if (output_dir != "") {
					output = output_dir + "\\" + output_file;
					out = 1;
				}
				else {
					output_site = getkeyd(pt,"Output.site","");
					if (output_site != "") {
						output_login = getkey(pt, "Output.login");
						output_password = getkey(pt, "Output.password");
						output = output_site + "/" + output_file;
						out = 2;
					}
				}

				src_clear_day = getkey(pt, "Icons_src.clear_day");
				src_clear_night = getkey(pt, "Icons_src.clear_night");
				src_rain = getkey(pt, "Icons_src.rain");
				src_rain_2 = getkey(pt, "Icons_src.rain_2");
				src_snow = getkey(pt, "Icons_src.snow");
				src_snow_2 = getkey(pt, "Icons_src.snow_2");
				src_sleet = getkey(pt, "Icons_src.sleet");
				src_wind = getkey(pt, "Icons_src.wind");
				src_fog = getkey(pt, "Icons_src.fog");
				src_cloudy = getkey(pt, "Icons_src.cloudy");
				src_partly_cloudy_day = getkey(pt, "Icons_src.partly_cloudy_day");
				src_partly_cloudy_night = getkey(pt, "Icons_src.partly_cloudy_night");

				crc_clear_day = getkey(pt, "Icons_crc.clear_day");
				crc_clear_night = getkey(pt, "Icons_crc.clear_night");
				crc_rain = getkey(pt, "Icons_crc.rain");
				crc_rain_2 = getkey(pt, "Icons_crc.rain_2");
				crc_snow = getkey(pt, "Icons_crc.snow");
				crc_snow_2 = getkey(pt, "Icons_crc.snow_2");
				crc_sleet = getkey(pt, "Icons_crc.sleet");
				crc_wind = getkey(pt, "Icons_crc.wind");
				crc_fog = getkey(pt, "Icons_crc.fog");
				crc_cloudy = getkey(pt, "Icons_crc.cloudy");
				crc_partly_cloudy_day = getkey(pt, "Icons_crc.partly_cloudy_day");
				crc_partly_cloudy_night = getkey(pt, "Icons_crc.partly_cloudy_night");

			}
			else 
				if (vm.count("config")) {
					string config = vm["config"].as<string>();
					if (config.empty()) config = "config.ini";
					bpt::ptree pt;
					bpt::ini_parser::read_ini(config, pt);
					provider_name = getkey(pt, "Provider.name");
//					provider_id = getkey(pt, "Provider.id");
					secret_key = getkey(pt, "Provider.key");

//					output_type = getkeyd(pt, "Output.type","XML");
					output_file = getkeyd(pt, "Output.file", "Weather.xmltrf");
					output_dir = getkeyd(pt, "Output.dir", "");
					if (output_dir != "") {
						output = output_dir + "\\" + output_file;
						out = 1;
					}
					else {
						output_site = getkeyd(pt, "Output.site", "");
						if (output_site != "") {
							output_login = getkey(pt, "Output.login");
							output_password = getkey(pt, "Output.password");
							output = output_site + "/" + output_file;
							out = 2;
						}
					}

					src_clear_day = getkey(pt, "Icons_src.clear_day");
					src_clear_night = getkey(pt, "Icons_src.clear_night");
					src_rain = getkey(pt, "Icons_src.rain");
					src_rain_2 = getkey(pt, "Icons_src.rain_2");
					src_snow = getkey(pt, "Icons_src.snow");
					src_snow_2 = getkey(pt, "Icons_src.snow_2");
					src_sleet = getkey(pt, "Icons_src.sleet");
					src_wind = getkey(pt, "Icons_src.wind");
					src_fog = getkey(pt, "Icons_src.fog");
					src_cloudy = getkey(pt, "Icons_src.cloudy");
					src_partly_cloudy_day = getkey(pt, "Icons_src.partly_cloudy_day");
					src_partly_cloudy_night = getkey(pt, "Icons_src.partly_cloudy_night");

					crc_clear_day = getkey(pt, "Icons_crc.clear_day");
					crc_clear_night = getkey(pt, "Icons_crc.clear_night");
					crc_rain = getkey(pt, "Icons_crc.rain");
					crc_rain_2 = getkey(pt, "Icons_crc.rain_2");
					crc_snow = getkey(pt, "Icons_crc.snow");
					crc_snow_2 = getkey(pt, "Icons_crc.snow_2");
					crc_sleet = getkey(pt, "Icons_crc.sleet");
					crc_wind = getkey(pt, "Icons_crc.wind");
					crc_fog = getkey(pt, "Icons_crc.fog");
					crc_cloudy = getkey(pt, "Icons_crc.cloudy");
					crc_partly_cloudy_day = getkey(pt, "Icons_crc.partly_cloudy_day");
					crc_partly_cloudy_night = getkey(pt, "Icons_crc.partly_cloudy_night");
				}
				else {
//					flog << curr_time() << "Конфигурационный файл необходим" << endl;
					throw runtime_error("configuration file is required, aborting");
				}

			CURL *curl;
			CURLcode res;
			char errbuf[CURL_ERROR_SIZE];
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
//				flog << curr_time() << list << ": Не могу открыть файл" << endl;
				throw runtime_error(list + ": cannot open file, aborting"); 
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
				string err;

//   		    if (((i % 10) == 0) && (i != 0)) Sleep(60000);

				string surl = "https://" + provider_name + "/forecast/" + secret_key + "/" + it->lat + "," + it->lon + "?units=si";
				char *url = new char[surl.length() + 1];
				strcpy(url, surl.c_str());
				int attempt = 0;
				string readBuffer;

				do {

					errbuf[0] = 0;
					curl = curl_easy_init();

					if (curl) {

//						flog << curr_time() << ": Город: " << it->city_name << " Широта: " << it->lat << " Долгота: " << it->lon << endl;

						curl_easy_setopt(curl, CURLOPT_URL, url);
						curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
						curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
						curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

						res = curl_easy_perform(curl);	

						if (res != CURLE_OK) {

							size_t len = strlen(errbuf);
							flog << to_string(res) << endl;
							if (len) {
								string err(errbuf);
								flog << curr_time() << ": (" << it->city_name << ") " << err << endl;
							}
							else
								flog << curr_time() << ": (" << it->city_name << ") " << curl_easy_strerror(res) << endl;

							attempt++;
							Sleep(10000);
						}
						else {
							break;
						}
					}
					else {
						throw runtime_error("Unknown error, aborting");
					}

					curl_easy_cleanup(curl);

				} while (attempt < 3);

				if (attempt == 3) {
//					flog << curr_time() << "Ошибка соединения" << endl;
					throw runtime_error("failed to connect, aborting");
				}

				boost::property_tree::ptree pt2;
				stringstream ss(readBuffer);

				time(&seconds);
				sec << seconds;
				string sc = sec.str();
				string tmp_json_file = sc + "_" + it->city_name;
				boost::filesystem::ofstream fjson(tmp_json_file);
				fjson << ss.rdbuf();

				boost::property_tree::json_parser::read_json(ss, pt2);


				if (pt2.count("code")) {
					flog << curr_time() << ": (" << it->city_name << ") " << ": Код ошибки:" << pt2.get<string>("code") << " Текст ошибки: " << pt2.get<string>("error") << endl;
					throw runtime_error("The request failed, aborting");
 				}

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
				float precip = pt2.get<float>("currently.precipIntensity");

				if (icon == "clear-day") { 
					current_weather.icon_src = src_clear_day;
					current_weather.icon_crc = crc_clear_day;
				}
				if (icon == "clear-night") {
					current_weather.icon_src = src_clear_night;
					current_weather.icon_crc = crc_clear_night;
				}
				if (icon == "rain") {
					if ( precip >= 0.4) {
						current_weather.icon_src = src_rain_2;
						current_weather.icon_crc = crc_rain_2;
					}
					else {
						current_weather.icon_src = src_rain;
						current_weather.icon_crc = crc_rain;
					}
				}
				if (icon == "snow") {
					if ( precip >= 0.4) {
						current_weather.icon_src = src_snow_2;
						current_weather.icon_crc = crc_snow_2;
					}
					else {
						current_weather.icon_src = src_snow;
						current_weather.icon_crc = crc_snow;
					}
				}
				if (icon == "sleet") {
					current_weather.icon_src = src_sleet;
					current_weather.icon_crc = crc_sleet;
				}
				if (icon == "wind") {
					current_weather.icon_src = src_wind;
					current_weather.icon_crc = crc_wind;
				}
				if (icon == "fog") {
					current_weather.icon_src = src_fog;
					current_weather.icon_crc = crc_fog;
				}
				if (icon == "cloudy") {
					current_weather.icon_src = src_cloudy;
					current_weather.icon_crc = crc_cloudy;
				}
				if (icon == "partly-cloudy-day") {
					current_weather.icon_src = src_partly_cloudy_day;
					current_weather.icon_crc = crc_partly_cloudy_day;
				}
				if (icon == "partly-cloudy-night") {
					current_weather.icon_src = src_partly_cloudy_night;
					current_weather.icon_crc = crc_partly_cloudy_night;
				}

				current_weather.temperature = to_string(lround(stoi(pt2.get<string>("currently.temperature"))));
			
				vec_wh hourly_weather;

				BOOST_FOREACH(auto &v, pt2.get_child("hourly.data"))
				{
					weather_hourly tmp2;
					assert(v.first.empty()); 
					tmp2.utime = v.second.get<string>("time");
					tmp2.hour = (((stoi(tmp2.utime)) % 86400) / 3600 + 8) % 24;
					string icon = v.second.get<string>("icon");
					float precip = v.second.get<float>("precipIntensity");

					if (icon == "clear-day") {
						tmp2.icon_src = src_clear_day;
						tmp2.icon_crc = crc_clear_day;
					}
					if (icon == "clear-night") {
						tmp2.icon_src = src_clear_night;
						tmp2.icon_crc = crc_clear_night;
					}
					if (icon == "rain") {
						if (precip >= 0.4) {
							tmp2.icon_src = src_rain_2;
							tmp2.icon_crc = crc_rain_2;
						}
						else {
							tmp2.icon_src = src_rain;
							tmp2.icon_crc = crc_rain;
						}
					}
					if (icon == "snow") {
						if (precip >= 0.4) {
							tmp2.icon_src = src_snow_2;
							tmp2.icon_crc = crc_snow_2;
						}
						else {
							tmp2.icon_src = src_snow;
							tmp2.icon_crc = crc_snow;
						}
					}
					if (icon == "sleet") {
						tmp2.icon_src = src_sleet;
						tmp2.icon_crc = crc_sleet;
					}
					if (icon == "wind") {
						tmp2.icon_src = src_wind;
						tmp2.icon_crc = crc_wind;
					}
					if (icon == "fog") {
						tmp2.icon_src = src_fog;
						tmp2.icon_crc = crc_fog;
					}
					if (icon == "cloudy") {
						tmp2.icon_src = src_cloudy;
						tmp2.icon_crc = crc_cloudy;
					}
					if (icon == "partly-cloudy-day") {
						tmp2.icon_src = src_partly_cloudy_day;
						tmp2.icon_crc = crc_partly_cloudy_day;
					}
					if (icon == "partly-cloudy-night") {
						tmp2.icon_src = src_partly_cloudy_night;
						tmp2.icon_crc = crc_partly_cloudy_night;
					}

					tmp2.temperature = to_string(lround(stoi(v.second.get<string>("temperature"))));
					hourly_weather.push_back(tmp2);

				}

				string current_w;
				string first_w;
				string second_w;
				string tmp3;
				string tmp4;
				string tmp5;

				if (stof(current_weather.temperature) > 0)
					tmp3 = "+";
				else
					tmp3 = "";

				if (stof(hourly_weather[15 - (current_weather.hour % 12)].temperature) > 0)
					tmp4 = "+";
				else
					tmp4 = "";

				if (stof(hourly_weather[27 - (current_weather.hour % 12)].temperature) > 0)
					tmp5 = "+";
				else
					tmp5 = "";

				current_w = "Сейчас: %3Cimg src=%22" + current_weather.icon_src + "%22 /%3E " + tmp3 + current_weather.temperature + "°";

					if ((current_weather.hour >= 0) && (current_weather.hour <= 11)) {
						first_w  = "%0D%0AДнем: %3Cimg src=%22" + hourly_weather[15 - (current_weather.hour % 12)].icon_src + "%22 /%3E " + tmp4 + hourly_weather[15 - (current_weather.hour % 12)].temperature + "°";
						second_w = "%0D%0AНочью: %3Cimg src=%22" + hourly_weather[27 - (current_weather.hour % 12)].icon_src + "%22 /%3E " + tmp5 + hourly_weather[27 - (current_weather.hour % 12)].temperature + "°";
					}
					else {
						first_w  = "%0D%0AНочью: %3Cimg src=%22" + hourly_weather[15 - (current_weather.hour % 12)].icon_src + "%22 /%3E " + tmp4 + hourly_weather[15 - (current_weather.hour % 12)].temperature + "°";
						second_w = "%0D%0AЗавтра: %3Cimg src=%22" + hourly_weather[27 - (current_weather.hour % 12)].icon_src + "%22 /%3E " + tmp5 + hourly_weather[27 - (current_weather.hour % 12)].temperature + "°";
					}

				i++;
				sdata2.put("<xmlattr>.name", "#Name");
				sdata2.put_value(it->city_name);
				sdata1.add_child("sdata", sdata2);

				string comments;
				comments = current_w + first_w + second_w;
				sdata2.put("<xmlattr>.name", "#Comments");
				sdata2.put_value(comments);
				sdata1.add_child("sdata", sdata2);

				string labeldisp;
				labeldisp = it->city_name + "%0D%0A" + tmp3 + current_weather.temperature + "°";
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

				string libcrc = current_weather.icon_crc;
 				sign.put("<xmlattr>.x", 0);
				sign.put("<xmlattr>.y", 0);
				sign.put("<xmlattr>.libcrc", libcrc);
				signs.add_child("sign", sign);
				obj.add_child("signs", signs);

				double EarthMapSize = 134217728;
				double gx = stod(it->lat);
				double gy = stod(it->lon);
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

				fjson.close();

			}
			trf.add_child("objs", objs);
			pt3.add_child("trf", trf);

			ofstream ofile(output);
			string tmp_name = "tmp_" + curr_date();
			ofstream outs(tmp_name);

			FILE * tmp;
			struct stat	file_info;

			auto settings = bpt::xml_writer_make_settings<string>('\t', 1, "windows-1251");

			char *ourl = new char[output.length() + 1];
			strcpy(ourl, output.c_str());
			char *olog = new char[output_login.length() + 1];
			strcpy(olog, output_login.c_str());
			char *opas = new char[output_password.length() + 1];
			strcpy(opas, output_password.c_str());

			switch (out)
			{
				case 1:
					bpt::write_xml(ofile, pt3, settings);
					ofile.close();
					break;
				case 2:
					bpt::write_xml(outs, pt3, settings);
					outs.close();

					stat(tmp_name.c_str(), &file_info);
					tmp = fopen(tmp_name.c_str(), "rb");

					curl = curl_easy_init();
					errbuf[0] = 0;
					if (curl) {
//						curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
						curl_easy_setopt(curl, CURLOPT_READFUNCTION, ReadCallback);
						curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
						curl_easy_setopt(curl, CURLOPT_URL, ourl);
						curl_easy_setopt(curl, CURLOPT_READDATA, tmp);
						curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_info.st_size);
						curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_BASIC);
						curl_easy_setopt(curl, CURLOPT_USERNAME, olog);
						curl_easy_setopt(curl, CURLOPT_PASSWORD, opas);
						curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
						res = curl_easy_perform(curl);

						if (res != CURLE_OK) {

							size_t len = strlen(errbuf);
							flog << to_string(res) << endl;
							if (len) {
								string err(errbuf);
								flog << curr_time() << ": (" << output_file << ") " << err << endl;
							}
							else
								flog << curr_time() << ": (" << output_file << ") " << curl_easy_strerror(res) << endl;

							throw runtime_error("failed to put file, aborting");

						}
					}
					else {
						throw runtime_error("Unknown error, aborting");
					}

					fclose(tmp);
					if (remove(tmp_name.c_str()) != 0)  throw runtime_error("file [" + tmp_name + "] deletion error, aborting");
					curl_easy_cleanup(curl);
					break;
				default:
					bpt::write_xml(cout, pt3, settings);
					break;
			}

			
			curl_global_cleanup();
			flog << curr_time() << ": Done" << endl;
			return 0;
		}
		catch (runtime_error& e) {
			flog << curr_time() << ": " << e.what() << endl;
			return 1;
		}
		}

