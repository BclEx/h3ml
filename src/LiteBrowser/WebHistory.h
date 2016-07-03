#pragma once

typedef std::vector<std::wstring> webHistory_vector;

class WebHistory
{
	webHistory_vector _items;
	webHistory_vector::size_type _current_item;
public:
	WebHistory();
	virtual ~WebHistory();

	void UrlOpened(const std::wstring &url);
	bool Back(std::wstring &url);
	bool Forward(std::wstring &url);
};