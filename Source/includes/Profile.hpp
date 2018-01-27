// Include Guard
#ifndef __PROFILE_H_INCLUDED__
#define __PROFILE_H_INCLUDED__
//=================================


#include <string>
#include <vector>

#include "./json.hpp"

#include "./Zone.hpp"

using nlohmann::json;

class Profile
{
private:
    unsigned int id;
    std::string name;
    std::string description;
    std::vector<Zone*> zones;

public:
    // Constructors
    Profile();
    Profile(const Profile& p);

    // Copy
    void copy(const Profile& p);

    // Accessors
    unsigned int get_id() const;
    std::string get_name() const;
    std::string get_description() const;

    // Mutators
    void set_id(unsigned int);
    void set_name(std::string);
    void set_description(std::string);

    // CRUD
    void add_zone(Zone* newZone);
    std::vector<Zone*> get_zones() const;
    void delete_zone(Zone* zone);

};

// JSON
void to_json(json& j, const Profile& p);
void from_json(const json& j, Profile& p);


#endif //__PROFILE_H_INCLUDED__
