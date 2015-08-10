#pragma once

#include <regex>
#include <string>

#include "string_helper.h"

std::string regex_escape(const std::string& string_to_escape)
{
	static const std::regex regexEscape("[.^$|()\\[\\]{}*+?\\\\]");
	const std::string rep("\\\\&");
	
	std::string result = std::regex_replace(
		string_to_escape, 
		regexEscape, 
		rep, 
		std::regex_constants::match_default | std::regex_constants::format_sed);

	return result;
}

std::string WildcardToRegex(std::string const& wildcardStr)
{
	auto& str = regex_escape(wildcardStr);
	StringReplace<std::string>(str, "\\*", ".*");
	StringReplace<std::string>(str, "\\?", ".");

	return "^" + str + "$";
}
