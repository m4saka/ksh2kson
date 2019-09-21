#include <iostream>
#include <nlohmann/json.hpp>
#include "ksh/playable_chart.hpp"

using json = nlohmann::json;

const char *getDifficultyName(const std::string & str)
{
    if (str == "light")
    {
        return "Light";
    }
    else if (str == "challenge")
    {
        return "Challenge";
    }
    else if (str == "extended")
    {
        return "Extended";
    }
    else
    {
        return "Infinite";
    }
}

const char *getDifficultyShortName(const std::string & str)
{
    if (str == "light")
    {
        return "LT";
    }
    else if (str == "challenge")
    {
        return "CH";
    }
    else if (str == "extended")
    {
        return "EX";
    }
    else
    {
        return "IN";
    }
}

int getDifficultyIdx(const std::string & str)
{
    if (str == "light")
    {
        return 0;
    }
    else if (str == "challenge")
    {
        return 1;
    }
    else if (str == "extended")
    {
        return 2;
    }
    else
    {
        return 3;
    }
}

int main(int argc, char *argv[])
{
    if (argc >= 2)
    {
        for (int i = 1; i < argc; ++i)
        {
            ksh::PlayableChart chart(argv[i]);

            // TODO: Convert to kson
            json kson = {
                { "version", "1.0.0" },
                { "meta", {
                    { "title", chart.metaData.at("title") },
                    { "title_translit", {} },
                    { "subtitle", {} },
                    { "artist", chart.metaData.at("artist") },
                    { "artist_translit", {} },
                    { "chart_author", chart.metaData.at("effect") },
                    { "difficulty", {
                        { "name", getDifficultyName(chart.metaData.at("difficulty")) },
                        { "short_name", getDifficultyShortName(chart.metaData.at("difficulty")) },
                        { "idx", getDifficultyIdx(chart.metaData.at("difficulty")) },
                    }},
                    { "level", std::stoi(chart.metaData.at("level")) },
                    { "disp_bpm", {} },
                    { "std_bpm", {} },
                    { "jacket_filename", chart.metaData.at("jacket") },
                    { "jacket_author", chart.metaData.at("illustrator") },
                    { "information", {} },
                }},
                { "beat", {} },
                { "gauge", {} },
                { "note", {} },
                { "audio", {} },
                { "camera", {} },
                { "bg", {} },
                { "impl", {} },
            };

            // TODO: Save to file
            std::cout << kson << std::endl;
        }
    }
    else
    {
        std::cerr <<
            "Usage:\n"
            "./ksh2kson [ksh file]" << std::endl;
    }
    return 0;
}
