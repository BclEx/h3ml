#include "globals.h"
#include "WebHistory.h"

WebHistory::WebHistory()
{
	_current_item = 0;
}

WebHistory::~WebHistory()
{
}

void WebHistory::UrlOpened(const std::wstring &url)
{
	if (!_items.empty()) {
		if (_current_item != _items.size() - 1) {
			if (_current_item > 0 && _items[_current_item - 1] == url)
				_current_item--;
			else if (_current_item < _items.size() - 1 && _items[_current_item + 1] == url)
				_current_item++;
			else {
				_items.erase(_items.begin() + _current_item + 1, _items.end());
				_items.push_back(url);
				_current_item = _items.size() - 1;
			}
		}
		else {
			if (_current_item > 0 && _items[_current_item - 1] == url)
				_current_item--;
			else {
				_items.push_back(url);
				_current_item = _items.size() - 1;
			}
		}
	}
	else {
		_items.push_back(url);
		_current_item = _items.size() - 1;
	}
}

bool WebHistory::Back(std::wstring &url)
{
	if (_items.empty()) return false;
	if (_current_item > 0) {
		url = _items[_current_item - 1];
		return true;
	}
	return false;
}

bool WebHistory::Forward(std::wstring &url)
{
	if (_items.empty()) return false;
	if (_current_item < _items.size() - 1) {
		url = _items[_current_item + 1];
		return true;
	}
	return false;
}
