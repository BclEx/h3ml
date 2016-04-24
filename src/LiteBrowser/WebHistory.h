#pragma once

typedef std::vector<std::wstring> string_vector;

class WebHistory
{
	string_vector _items;
	string_vector::size_type _current_item;
public:
	WebHistory();
	virtual ~WebHistory();

	void UrlOpened(const std::wstring &url);
	bool Back(std::wstring &url);
	bool Forward(std::wstring &url);
};