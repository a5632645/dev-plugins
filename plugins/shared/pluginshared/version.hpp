#pragma once

namespace pluginshared::version {
/**
 * @brief make a version string to code, such as "0.0.1" to [0,0,1]
 * @note if not exits that bit, that bit will be 0
 * @return [major, minor, patch]
 */
static inline std::tuple<int, int, int> ParseVersionString(const juce::String& versionString)
{
    juce::StringArray tokens;
    tokens.addTokens(versionString, ".", "");

    int major = 0, minor = 0, patch = 0;

    if (tokens.size() > 0)
        major = tokens[0].getIntValue();
    if (tokens.size() > 1)
        minor = tokens[1].getIntValue();
    if (tokens.size() > 2)
        patch = tokens[2].getIntValue();

    return std::make_tuple(major, minor, patch);
}
}