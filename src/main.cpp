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

const char *getLaneSpinCamPatternName(LaneSpin::Type type)
{
    switch (type)
    {
    case LaneSpin::Type::Normal:
        return "spin";
    case LaneSpin::Type::Half:
        return "half_spin";
    case LaneSpin::Type::Swing:
        return "swing";
    default:
        return "";
    }
}

json getKsonMetaData(const ksh::PlayableChart & chart)
{
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

    return metaData;
}

json getKsonBeatData(const ksh::PlayableChart & chart)
{
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

    return beatData;
}

json getKsonGaugeData(const ksh::PlayableChart & chart)
{
    json gaugeData = {
        { "total", {} },
    };

    if (chart.metaData.count("total"))
    {
        gaugeData["total"] = std::stod(chart.metaData.at("total"));
    }

    return gaugeData;
}

json getKsonNoteData(const ksh::PlayableChart & chart)
{
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
                // Determine if the laser section is in wide mode
                const std::string kshWideKey = (i == 0) ? "laserrange_l" : "laserrange_r";
                const auto & options = chart.positionalOptions();
                int wide = (options.count(kshWideKey) && options.at(kshWideKey).count(y) && options.at(kshWideKey).at(y) == "2x") ? 2 : 1;

                // Insert a laser section
                noteData["laser"][i].push_back({
                    { "y", y },
                    { "v", {} },
                    { "wide", wide },
                });
                sectionOffsetY = y;
                ++sectionIdx;

                // First point in the laser section
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

    return noteData;
}

json getKsonAudioData(const ksh::PlayableChart & chart)
{
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

    return audioData;
}

json getKsonCameraData(const ksh::PlayableChart & chart)
{
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
            { "pattern", {
                { "def", {} },
                { "pulse_event", {} },
                { "note_event", {} },
            }},
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

    for (std::size_t i = 0; i < 2; ++i)
    {
        int sectionIdx = -1;
        Measure prevY = -1;
        int pointIdx = 0;
        for (const auto & [ y, laserNote ] : chart.laserLane(i))
        {
            if (y != prevY)
            {
                ++sectionIdx;
                pointIdx = 0;
            }
            if (laserNote.laneSpin.type != LaneSpin::Type::NoSpin)
            {
                cameraData["cam"]["pattern"]["note_event"][getLaneSpinCamPatternName(laserNote.laneSpin.type)]["laser"].push_back({
                    { "lane", i },
                    { "sec", sectionIdx },
                    { "idx", pointIdx },
                    { "v", {
                        { "l", laserNote.laneSpin.length },
                        { "scale", (laserNote.laneSpin.direction == LaneSpin::Direction::Left) ? -1.0 : 1.0 },
                    }},
                });
            }
            prevY = y + laserNote.length;
            ++pointIdx;
        }
    }

    return cameraData;
}

json getKsonBgData(const ksh::PlayableChart & chart)
{
    return {};
}

json convertToKson(const ksh::PlayableChart & chart)
{
    return json{
        { "version", "1.0.0" },
        { "meta", getKsonMetaData(chart) },
        { "beat", getKsonBeatData(chart) },
        { "gauge", getKsonGaugeData(chart) },
        { "note", getKsonNoteData(chart) },
        { "audio", getKsonAudioData(chart) },
        { "camera", getKsonCameraData(chart) },
        { "bg", getKsonBgData(chart) },
        { "impl", {} },
    };
}

int main(int argc, char *argv[])
{
    if (argc >= 2)
    {
        for (int i = 1; i < argc; ++i)
        {
            ksh::PlayableChart chart(argv[i]);
            auto kson = convertToKson(chart);

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
