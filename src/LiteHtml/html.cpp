#include "html.h"
#include "types.h"
#include "html_tag.h"
using namespace litehtml;

void litehtml::trim(tstring &s) 
{
	tstring::size_type pos = s.find_first_not_of(_t(" \n\r\t"));
	if (pos != tstring::npos)
		s.erase(s.begin(), s.begin() + pos);
	pos = s.find_last_not_of(_t(" \n\r\t"));
	if (pos != tstring::npos)
		s.erase(s.begin() + pos + 1, s.end());
}

void litehtml::lcase(tstring &s) 
{
	for(tstring::iterator i = s.begin(); i != s.end(); i++)
		(*i) = t_tolower(*i);
}

tstring::size_type litehtml::find_close_bracket(const tstring &s, tstring::size_type off, tchar_t open_b, tchar_t close_b)
{
	int cnt = 0;
	for (tstring::size_type i = off; i < s.length(); i++) {
		if (s[i] == open_b)
			cnt++;
		else if (s[i] == close_b) {
			cnt--;
			if (!cnt)
				return i;
		}
	}
	return tstring::npos;
}

int litehtml::value_index(const tstring &val, const tstring &strings, int defValue, tchar_t delim)
{
	if (val.empty() || strings.empty() || !delim)
		return defValue;

	int idx = 0;
	tstring::size_type deli_start = 0;
	tstring::size_type deli_end	= strings.find(delim, deli_start);
	tstring::size_type ite_len = 0;
	while (true) {
		ite_len = (deli_end == tstring::npos ? strings.length() - deli_start : deli_end - deli_start);
		if (ite_len == val.length())
			if (val == strings.substr(deli_start, ite_len))
				return idx;
		idx++;
		deli_start = deli_end;
		if (deli_start == tstring::npos) break;
		deli_start++;
		if (deli_start == strings.length()) break;
		deli_end = strings.find(delim, deli_start);
	}
	return defValue;
}

bool litehtml::value_in_list(const tstring &val, const tstring &strings, tchar_t delim)
{
	int idx = value_index(val, strings, -1, delim);
	return (idx >= 0);
}

void litehtml::split_string(const tstring &str, string_vector &tokens, const tstring &delims, const tstring &delims_preserve, const tstring &quote)
{
	if (str.empty() || (delims.empty() && delims_preserve.empty()))
		return;
	tstring all_delims = delims + delims_preserve + quote;
	tstring::size_type token_start = 0;
	tstring::size_type token_end = str.find_first_of(all_delims, token_start);
	tstring::size_type token_len = 0;
	tstring token;
	while (true) {
		while (token_end != tstring::npos && quote.find_first_of(str[token_end]) != tstring::npos) {
			if (str[token_end] == _t('('))
				token_end = find_close_bracket(str, token_end, _t('('), _t(')'));
			else if (str[token_end] == _t('['))
				token_end = find_close_bracket(str, token_end, _t('['), _t(']'));
			else if (str[token_end] == _t('{'))
				token_end = find_close_bracket(str, token_end, _t('{'), _t('}'));
			else
				token_end = str.find_first_of(str[token_end], token_end + 1);
			if (token_end != tstring::npos)
				token_end = str.find_first_of(all_delims, token_end + 1);
		}
		token_len = (token_end == tstring::npos ? tstring::npos : token_end - token_start);
		token = str.substr(token_start, token_len);
		if (!token.empty())
			tokens.push_back( token );
		if (token_end != tstring::npos && !delims_preserve.empty() && delims_preserve.find_first_of(str[token_end]) != tstring::npos)
			tokens.push_back( str.substr(token_end, 1) );
		token_start = token_end;
		if (token_start == tstring::npos) break;
		token_start++;
		if (token_start == str.length()) break;
		token_end = str.find_first_of(all_delims, token_start);
	}
}

void litehtml::join_string(tstring &str, const string_vector &tokens, const tstring &delims)
{
	tstringstream ss;
	for (size_t i=0; i<tokens.size(); ++i) {
		if (i != 0)
			ss << delims;
		ss << tokens[i];
	}
	str = ss.str();
}
