// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef MIDDLEWARE_COOKIE_PARSER_HPP
#define MIDDLEWARE_COOKIE_PARSER_HPP

//#include <ctime>
#include <time.h>
#include <ostream>

#include "middleware.hpp"
#include "cookie_jar.hpp"
#include "cookie.hpp"

// #include <cookie>

using namespace cookie;

namespace middleware {

/**
 * @brief A way to parse cookies: Reading cookies that the browser is sending to the server
 * @details
 */
class CookieParser : public server::Middleware {

public:

  CookieParser(std::shared_ptr<CookieJar> jar);

  ~CookieParser() = default;

  /**
   *
   */
  virtual void process(server::Request_ptr req, server::Response_ptr res, server::Next next) override;

private:

  // CookieParser takes a CookieJar as parameter (created in the service.cpp)
  std::shared_ptr<CookieJar> jar_;

  static const std::regex pattern;

  bool has_cookie(server::Request_ptr req) const noexcept;

  const std::string& read_cookies(server::Request_ptr req) const noexcept;

  bool set_sid_cookie(server::Response_ptr res) const noexcept;

  // Cookie parse(const std::string& cookie_data);

  std::set<Cookie> parse_from_client(const std::string& cookie_data);

  // std::string serialize(const Cookie& cookie) const;

  // bool clear_cookie(server::Response_ptr res);
  // bool clear_cookie(std::string& name);
    // remove Cookie from client

  void clear();

}; //< class CookieParser

/**--v----------- Implementation Details -----------v--**/

CookieParser::CookieParser(std::shared_ptr<CookieJar> jar)
  : jar_{move(jar)} {}

inline void CookieParser::process(server::Request_ptr req, server::Response_ptr res, server::Next next) {

  // Parse creates the cookie object from string values
  // Serialize/to_string creates the cookie string that is going into the header

  // Get the request's path
  std::string request_path = req->uri().path();

  if(not has_cookie(req)) {

    // Set a unique session identifier cookie:
    set_sid_cookie(res);

    // When the request has no cookies: do not parse cookie string, but go through jar_
    // and set all cookies that match this path:

    // Go through jar_ and add_header Set-Cookie for each:
    std::vector<Cookie> all_cookies = jar_->get_cookies();

    for(size_t i = 0; i < all_cookies.size(); i++) {
      Cookie c = all_cookies[i];
      std::string cookie_path = c.get_path();

      // If the path == / or the path matches the req's path,
      // we want to set the cookie in the response header:
      if(cookie_path == "/" or cookie_path == request_path) {
        res->add_header(http::header_fields::Response::Set_Cookie, c.to_string());
      }
    }

    // (When the developer creates cookies he can set options, f.ex. path)
    // If path not set, the default is / (everywhere on the website)

  } else {

    //CookieCollection cookie_collection;
    CookieJar req_cookies;

    // Get the cookies that already exists (sent in the request):
    std::string cookies_string = read_cookies(req);
    std::set<Cookie> existing_cookies = parse_from_client(cookies_string);




    // TODO:

    // Check if the user has a cookie with the name SID (identifier):
    std::set<Cookie>::iterator it = std::find_if(existing_cookies.begin(), existing_cookies.end(),
      [] (const Cookie& cookie) { return cookie.get_name() == "SID"; });

    if(it not_eq existing_cookies.end()) {

      printf("SID-cookie FOUND\n");

      // The request has a cookie named SID - want to make it accessible to the developer:

      // TODO:

      // Get the SID cookie from the request and add it to req->set_attribute so that the
      // developer can access it - ADD TO CookieCollection

      Cookie sid_cookie = *it;

      /*auto cookie_attr = std::make_shared<Cookie>(sid_cookie);
      req->set_attribute(cookie_attr);*/
      req_cookies.add(sid_cookie);

      // BUT THE DEVELOPER GETS NO ADDITIONAL INFORMATION THAN THE SID-COOKIES NAME AND VALUE
      // THAT IS OK SINCE THE COOKIEPARSER IS RESPONSIBLE FOR CREATING IT....?
      // CANNOT CHANGE THE COOKIE'S OPTIONS - SHOULD THE DEVELOPER BE ABLE TO CHANGE THESE..?

    } else {  // if the user has no SID-cookie, we create one:

      printf("SID-cookie NOT FOUND\n");

      set_sid_cookie(res);
    }






    // TODO
    // Go through jar_ and add_header Set-Cookie for each cookie that
    // doesn't exist in the request -
    // DO WE WANT TO DO THIS?:
    std::vector<Cookie> all_cookies = jar_->get_cookies();

    for(size_t i = 0; i < all_cookies.size(); i++) {
      Cookie c = all_cookies[i];

      if(existing_cookies.find(c) not_eq existing_cookies.end()) {
        // If the cookie already exists, we just continue the for-loop,
        // but first add the Cookie to the request so that the developer
        // can access it in service.cpp (TODO: Add Cookie c to CookieCollection
        // instead of adding it to the request here. Then later add the
        // CookieCollection to the request.)

        /*auto cookie_attr = std::make_shared<Cookie>(c);
        req->set_attribute(cookie_attr);*/
        req_cookies.add(c);

        continue;
      }




      // TESTING: TODO: NOT SET lang-COOKIE
      /*ADDED: TODO: If a cookie in existing_cookies has a name equal to a cookie's name
      in jar_, we will not set the cookie ??
      TESTING WITH NAME = "lang" */
      /*if(all_cookies[i].get_name() == "lang")
        continue;*/






      // If the cookie doesn't exist and the path to the cookie in jar_
      // matches the request's path or is /, we want to set the cookie in
      // the response header:

      std::string cookie_path = c.get_path();

      if(cookie_path == "/" or cookie_path == request_path) {
        res->add_header(http::header_fields::Response::Set_Cookie, c.to_string());
        // c.to_string() returns the serialized cookie: is a valid set_cookie header string
      }
    }

    /*auto cookie_attr = std::make_shared<Cookie>(c);
    res->set_attribute(cookie_attr);*/
    auto jar_attr = std::make_shared<CookieJar>(req_cookies);
    req->set_attribute(jar_attr);
  }

  return (*next)();
}

const std::regex CookieParser::pattern{"[^;]+"};

inline bool CookieParser::has_cookie(server::Request_ptr req) const noexcept {
  return req->has_header(http::header_fields::Request::Cookie);
}

inline const std::string& CookieParser::read_cookies(server::Request_ptr req) const noexcept {
  return req->header_value(http::header_fields::Request::Cookie);
}

bool CookieParser::set_sid_cookie(server::Response_ptr res) const noexcept {
  try {

    // TODO

    // Use hash function of what values ? (unique and random)
    //std::string sid = std::hash<std::string> ...;
    //Cookie sid_cookie{"SID", sid, "Secure", "true", "HttpOnly", "true"};
    // Set expires or max_age? If not: Cookie only set for the user's session

    // JUST FOR TESTING:
    //Cookie sid_cookie{"SID", "1f14a3s4dj2919", std::vector<std::string>{"Secure", "true", "HttpOnly", "true"}};
    Cookie sid_cookie{"SID", "1f14a3s4dj2919"};

    // NOT ADD sid_cookie TO JAR_?

    res->add_header(http::header_fields::Response::Set_Cookie, sid_cookie.to_string());
    // sid_cookie.to_string() returns the serialized cookie: is a valid set_cookie header string

    return true;

  } catch (CookieException& ce) {
    printf("Unable to create SID-cookie!\n");
    return false;
  }
}

/*Cookie CookieParser::parse(const std::string& cookie_data) {

}*/

std::set<Cookie> CookieParser::parse_from_client(const std::string& cookie_data) {
  // From the client we only get name=value; for each cookie

  std::set<Cookie> cookies;

  if(cookie_data.empty())
    throw CookieException{"Cannot parse empty cookie-string!"};

  auto position = std::sregex_iterator(cookie_data.begin(), cookie_data.end(), pattern);
  auto end = std::sregex_iterator();

  for (std::sregex_iterator i = position; i != end; ++i) {
    std::smatch pair = *i;
    std::string pair_str = pair.str();

    // Remove all empty spaces:
    pair_str.erase(std::remove(pair_str.begin(), pair_str.end(), ' '), pair_str.end());

    size_t pos = pair_str.find("=");
    std::string name = pair_str.substr(0, pos);
    std::string value = pair_str.substr(pos + 1);

    Cookie c{name, value};

    if(not cookies.insert(c).second)
      throw CookieException{"Could not add cookie with name " + name + " and value " + value + " to the set!"};
  }

  return cookies;
}

/* Not necessary? (can just call cookie.to_string() in process method)
std::string CookieParser::serialize(const Cookie& cookie) const {
  return cookie.to_string();
}*/

// TODO: Finish this
void CookieParser::clear() {
  jar_->clear();
}

/*bool CookieParser::clear_cookie(server::Response_ptr res) {

  // find Cookie in CookieJar (?) and alter the expires field (in the past)
*/
  /* try catch
  cookie::Cookie c{"name", "value"};

  //time_t time = time(NULL);  // now - 1
  time_t t = time(0);
  struct tm* pmt = gmtime(&t);

  if(pmt == NULL)
    return false;
  */
/*
  //strftime(%Y-%m-%dT%H:%M:%S%Z);

  //strftime(%a );

  //std::string time_string{std::to_string(pmt->tm_hour % 24) + ":" + std::to_string(pmt->tm_min) + ":"};

  // if < 10 (timer, min, sek): legge til 0 foran

  // Format on Expires value: Sun, 01-Jan-2016 22:00:02 GMT

  // Check out to_string in src/hw/cmos.cpp (IncludeOS)
*/
  //std::vector<std::string> v{"Expires", time_string};
  /*c.set_expires(time);
  std::string cs = c.serialize();
  res->add_header(http::header_fields::Response::Set_Cookie, cs);*/
/*
  // remove from CookieJar (if successful clear...?)

  // php: setcookie("yourcookie","yourvalue",time()-1);
  // it expired so it's deleted
  // RFC 6265 5.3: A cookie is "expired" if the cookie has an expiry date in the past.

  return true;
}*/

/**--^----------- Implementation Details -----------^--**/

} //< namespace middleware

#endif //< MIDDLEWARE_COOKIE_PARSER_HPP
