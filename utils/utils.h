bool isNumeric(const String& str) {
    for (size_t i = 0; i < str.length(); i++) {
        if (!isdigit(str.charAt(i))) {
            return false;
        }
    }
    return true;
}