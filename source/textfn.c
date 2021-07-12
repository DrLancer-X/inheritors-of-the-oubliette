// Simple text functions, because stdio is too heavy for GBA (uses up too much iwram)
void txt_erase(char *buf)
{
	buf[0] = '\0';
}
void txt_set(char *buf, const char *txt)
{
	int pos = 0;
	const char *p = txt;
	while (*p != '\0')
		buf[pos++] = *(p++);
	buf[pos] = '\0';
}
int txt_len(const char *buf)
{
	for (int i = 0;; i++)
		if (buf[i] == '\0') return i;
}
void txt_append_str(char *buf, const char *append)
{
	int pos = txt_len(buf);
	const char *p = append;
	while (*p != '\0')
		buf[pos++] = *(p++);
	buf[pos] = '\0';
}
void txt_append_num(char *buf, int num)
{
	int pos = txt_len(buf);
	if (num < 0) {
		buf[pos++] = '-';
		num *= -1;
	}
	int div = 1000000000;
	while (div > 1) {
		if (num >= div) {
			break;
		}
		div /= 10;
	}
	while (div > 1) {
		buf[pos++] = '0' + num / div;
		num %= div;
		div /= 10;
	}
	buf[pos++] = '0' + num;
	buf[pos] = '\0';
}
