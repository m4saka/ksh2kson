#include <iostream>
#include <cstddef>
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

            json metaData = {
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
            };

            // If "t" is different from actual initial tempo or '-' is found in tempo, set disp_bpm to it
            // Note: tempo value is multiplied by 1000 and casted to int before comparison because the precision of ksh tempo is 0.001
            if (static_cast<int>(std::stod(chart.metaData.at("t")) * 1000) != static_cast<int>(chart.beatMap().tempo(0) * 1000) ||
                chart.metaData.at("t").find('-') != std::string::npos)
            {
                metaData["disp_bpm"] = chart.metaData.at("t");
            }

            if (chart.metaData.count("to"))
            {
                metaData["std_bpm"] = std::stod(chart.metaData.at("to"));
            }

            if (chart.metaData.count("information"))
            {
                metaData["information"] = chart.metaData.at("information");
            }

            json beatData = {
                { "bpm", {} },
                { "time_sig", {} },
                { "scroll_speed", {} },
                { "resolution", UNIT_MEASURE / 4 },
            };

            for (const auto & [ y, tempo ] : chart.beatMap().tempoChanges())
            {
                beatData["bpm"].push_back({
                    { "y", y },
                    { "v", tempo },
                });
            }

            for (const auto & [ y, timeSignature ] : chart.beatMap().timeSignatureChanges())
            {
                beatData["time_sig"].push_back({
                    { "y", y },
                    { "v", {
                        { "n", timeSignature.numerator },
                        { "d", timeSignature.denominator },
                    }},
                });
            }

            json gaugeData = {
                { "total", {} },
            };

            if (chart.metaData.count("total"))
            {
                gaugeData["total"] = std::stod(chart.metaData.at("total"));
            }

            json noteData = {};

            for (std::size_t i = 0; i < 4; ++i)
            {
                for (const auto & [ y, btNote ] : chart.btLane(i))
                {
                    noteData["bt"][i].push_back({
                        { "y", y },
                    });
                    if (btNote.length > 0)
                    {
                        noteData["bt"][i].back()["l"] = btNote.length;
                    }
                }
            }

            for (std::size_t i = 0; i < 2; ++i)
            {
                for (const auto & [ y, fxNote ] : chart.fxLane(i))
                {
                    noteData["fx"][i].push_back({
                        { "y", y },
                    });
                    if (fxNote.length > 0)
                    {
                        noteData["fx"][i].back()["l"] = fxNote.length;
                    }
                }
            }

            for (std::size_t i = 0; i < 2; ++i)
            {
                int sectionIdx = -1;
                Measure prevY = -1;
                Measure sectionOffsetY = -1;
                for (const auto & [ y, laserNote ] : chart.laserLane(i))
                {
                    if (y != prevY)
                    {
                        // First note in a laser section
                        noteData["laser"][i].push_back({
                            { "y", y },
                            { "v", {} },
                            { "wide", 1 }, // TODO: convert 2x wide section
                        });
                        sectionOffsetY = y;
                        ++sectionIdx;

                        noteData["laser"][i][sectionIdx]["v"].push_back({
                            { "ry", 0 },
                            { "v", static_cast<double>(laserNote.startX) / LaserNote::X_MAX },
                        });
                    }
                    if (laserNote.length <= UNIT_MEASURE / 32)
                    {
                        // Laser slams
                        noteData["laser"][i][sectionIdx]["v"].back()["vf"] = static_cast<double>(laserNote.endX) / LaserNote::X_MAX;
                    }
                    else
                    {
                        // Normal laser notes
                        noteData["laser"][i][sectionIdx]["v"].push_back({
                            { "ry", y + laserNote.length - sectionOffsetY },
                            { "v", static_cast<double>(laserNote.endX) / LaserNote::X_MAX },
                        });
                    }
                    prevY = y + laserNote.length;
                }
            }

            json audioData = {
                { "bgm", {
                    { "filename", chart.metaData.at("m") },
                    { "vol", 1.0 },
                    { "offset", std::stoi(chart.metaData.at("o")) },
                    { "preview_filename", {} },
                    { "preview_offset", std::stoi(chart.metaData.at("po")) },
                    { "preview_duration", std::stoi(chart.metaData.at("plength")) },
                }},
                { "key_sound", {} },
                { "audio_effect", {} },
            };

            if (chart.metaData.count("mvol"))
            {
                audioData["bgm"]["vol"] = std::stod(chart.metaData.at("mvol")) / 100.0;
            }

            json cameraData = {
                { "tilt", {
                    { "manual", {} },
                    { "keep", {} },
                }},
                { "cam", {
                    { "body", {
                        { "zoom", {} },
                        { "shift_x", {} },
                        { "rotation_x", {} },
                        { "rotation_z", {} },
                    }},
                    { "tilt_assign", {} },
                    { "pattern", {} },
                }},
            };

            for (const auto & [ y, zoom ] : chart.bottomLaneZooms())
            {
                cameraData["body"]["zoom"].push_back({
                    { "y", y },
                    { "v", zoom.first / 100.0 },
                });
                if (zoom.first != zoom.second)
                {
                    cameraData["body"]["zoom"].back()["vf"] = zoom.second / 100.0;
                }
            }

            for (const auto & [ y, zoom ] : chart.topLaneZooms())
            {
                cameraData["body"]["rotation_x"].push_back({
                    { "y", y },
                    { "v", zoom.first / 100.0 },
                });
                if (zoom.first != zoom.second)
                {
                    cameraData["body"]["rotation_x"].back()["vf"] = zoom.second / 100.0;
                }
            }

            for (const auto & [ y, zoom ] : chart.sideLaneZooms())
            {
                cameraData["body"]["shift_x"].push_back({
                    { "y", y },
                    { "v", zoom.first / 100.0 },
                });
                if (zoom.first != zoom.second)
                {
                    cameraData["body"]["shift_x"].back()["vf"] = zoom.second / 100.0;
                }
            }

            json bgData = {};

            json kson = {
                { "version", "1.0.0" },
                { "meta", metaData },
                { "beat", beatData },
                { "gauge", gaugeData },
                { "note", noteData },
                { "audio", audioData },
                { "camera", cameraData },
                { "bg", bgData },
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
