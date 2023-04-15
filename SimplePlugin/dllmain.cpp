#include "../plugin_sdk/plugin_sdk.hpp"
#include "twitch.h"
#include "ezreal.h"
#include "rengar.h"
#include "riven.h"""

PLUGIN_NAME("TrentAIO");
PLUGIN_TYPE(plugin_type::champion);
SUPPORTED_CHAMPIONS(champion_id::Ezreal, champion_id::Twitch, champion_id::Rengar, champion_id::Riven);
PLUGIN_API bool on_sdk_load(plugin_sdk_core* plugin_sdk_good)
{
    DECLARE_GLOBALS(plugin_sdk_good);

    switch (myhero->get_champion())
    {



    case champion_id::Ezreal:
        ezreal::load();
        break;
    case champion_id::Twitch:
        twitch::load();
        break;
    case champion_id::Rengar:
        rengar::load();
        break;
    case champion_id::Riven:
        riven::load();
        break;

    default:

        console->print("%s is not supported", myhero->get_model_cstr());
        return false;
    }

    console->print("%s loaded successfully.", myhero->get_model_cstr());
    return true;

}

PLUGIN_API void on_sdk_unload()
{
    switch (myhero->get_champion())
    {


    case champion_id::Ezreal:
        ezreal::unload();
        break;
    case champion_id::Twitch:
        twitch::unload();
        break;
    case champion_id::Rengar:
        rengar::unload();
        break;
    case champion_id::Riven:
        riven::unload();
        break;
    default:
        break;
    }
}
