/* 
 * File:   FFJSON.cpp
 * Author: Satya Gowtham Kudupudi
 * 
 * Created on November 29, 2013, 4:29 PM
 */

#include <string>
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <iostream>
#include <cstdlib>

#include "FFJSON.h"
#include "mystdlib.h"
#include "myconverters.h"
#include "logger.h"

using namespace std;

const char FFJSON::OBJ_STR[8][15] = {
	"UNDEFINED", "STRING", "XML", "NUMBER", "BOOL", "OBJECT", "ARRAY", "NUL"
};

FFJSON::FFJSON() {
	type = UNDEFINED;
	size = 0;
	qtype = NONE;
	etype = ENONE;
	val.pairs = NULL;
	val.boolean = false;
}

//FFJSON::FFJSON(std::ifstream file) {
//	std::string ffjson((std::istreambuf_iterator<char>(file)),
//			std::istreambuf_iterator<char>());
//	init(ffjson);
//}

FFJSON::FFJSON(OBJ_TYPE t) {
	type = UNDEFINED;
	qtype = NONE;
	etype = ENONE;
	if (t == OBJECT) {
		setType(OBJECT);
		val.pairs = new map<string, FFJSON*>;
	} else if (t == ARRAY) {
		setType(ARRAY);
		val.array = new vector<FFJSON*>();
	} else if (t == STRING) {
		setType(STRING);
	} else if (t == XML) {
		setType(XML);
	} else if (t == NUMBER) {
		setType(NUMBER);
	} else if (t == BOOL) {
		setType(BOOL);
	} else if (t == NUL) {
		setType(NUL);
	} else {
		throw Exception("UNDEFINED");
	}
}

FFJSON::FFJSON(const FFJSON& orig, COPY_FLAGS cf) {
	copy(orig, cf);
}

void FFJSON::copy(const FFJSON& orig, COPY_FLAGS cf) {
	setType(orig.getType());
	size = orig.size;
	type = orig.type;
	qtype = NONE;
	etype = ENONE;
	if (isType(NUMBER)) {
		val.number = orig.val.number;
	} else if (isType(STRING)) {
		val.string = (char*) malloc(orig.size + 1);
		memcpy(val.string, orig.val.string, orig.size);
		val.string[orig.size] = '\0';
		size = orig.size;
		if (cf == COPY_EFLAGS) {
			etype = orig.getEFlags();
		}
	} else if (isType(XML)) {
		val.string = (char*) malloc(orig.size + 1);
		memcpy(val.string, orig.val.string, orig.size);
		val.string[orig.size] = '\0';
		size = orig.size;
		if (cf == COPY_EFLAGS) {
			etype = orig.getEFlags();
		}
	} else if (isType(BOOL)) {
		val.boolean = type;
	} else if (isType(OBJECT)) {
		map<string, FFJSON*>::iterator i;
		i = orig.val.pairs->begin();
		val.pairs = new map<string, FFJSON*>();
		while (i != orig.val.pairs->end()) {
			FFJSON* fo;
			if (i->second != NULL)
				fo = new FFJSON(*i->second, cf);
			if (fo && ((cf == COPY_QUERIES&&!fo->isQType(QUERY_TYPE::NONE))
					|| !fo->isType(UNDEFINED))) {
				(*val.pairs)[i->first] = fo;
			} else {
				delete fo;
			}
			i++;
		}
		if (val.pairs->size() == 0) {
			delete val.pairs;
			val.pairs = NULL;
			type = UNDEFINED;
		};
	} else if (isType(ARRAY)) {
		int i = 0;
		val.array = new vector<FFJSON*>();
		bool matter = false;
		while (i < orig.val.array->size()) {
			FFJSON * fo = NULL;
			if ((*orig.val.array)[i] != NULL)
				fo = new FFJSON(*(*orig.val.array)[i]);
			if (fo && ((cf == COPY_QUERIES&&!fo->isQType(QUERY_TYPE::NONE))
					|| !fo->isType(UNDEFINED))) {
				(*val.array).push_back(fo);
				matter = true;
			} else {
				(*val.array).push_back(NULL);
				delete fo;
			}
			i++;
		}
		if (!matter) {
			delete val.array;
			val.array = NULL;
			type = UNDEFINED;
		}
	} else if (isType(NUL)) {
		setType(NUL);
		size = 0;
		val.boolean = false;
	} else if ((cf == COPY_QUERIES&&!isQType(QUERY_TYPE::NONE)) &&
			isType(UNDEFINED)) {
		qtype = orig.getQType();
	} else {
		setType(UNDEFINED);
		val.boolean = false;
	}
}

FFJSON::FFJSON(const string& ffjson, int* ci) {
	init(ffjson, ci);
}

void FFJSON::init(const std::string& ffjson, int* ci) {
	int i = (ci == NULL) ? 0 : *ci;
	int j = ffjson.length();
	type = UNDEFINED;
	qtype = NONE;
	etype = ENONE;
	size = 0;
	while (i < j) {
		if (ffjson[i] == '{') {
			setType(OBJECT);
			val.pairs = new map<string, FFJSON*>();
			i++;
			int objIdNail = i;
			string objId;
			while (i < j) {
				if (ffjson[i] == ':') {
					i++;
					try {
						objId = ffjson.substr(objIdNail, i - objIdNail - 1);
						trimWhites(objId);
						trimQuotes(objId);
						FFJSON* obj = new FFJSON(ffjson, &i);
						(*val.pairs)[objId] = obj;
						size++;
					} catch (Exception e) {

					}
				} else if (ffjson[i] == ',') {
					i++;
					objIdNail = i;
				} else if (ffjson[i] == '}') {
					i++;
					break;
				} else {
					i++;
				}
			}
			break;
		} else if (ffjson[i] == '[') {
			setType(ARRAY);
			size = 0;
			val.array = new vector<FFJSON*>();
			i++;
			int objNail = i;
			while (i < j) {
				try {
					FFJSON* obj = new FFJSON(ffjson, &i);
					if (obj->isType(NUL) && ffjson[i] == ']' && size == 0) {
						delete obj;
						obj = NULL;
					} else {
						if ((obj->isType(NUL) || obj->isType(UNDEFINED)) &&
								obj->isQType(NONE)) {
							delete obj;
							val.array->push_back(NULL);
						} else {
							val.array->push_back(obj);
						}
						size++;
					}
				} catch (Exception e) {

				}
				while (ffjson[i] != ',' && ffjson[i] != ']')i++;
				if (ffjson[i] == ',') {
					i++;
					objNail = i;
				} else if (ffjson[i] == ']') {
					i++;
					break;
				} else {
					i++;
				}
			}
			break;
		} else if (ffjson[i] == '"') {
			i++;
			int strNail = i;
			setType(STRING);
			while (i < j) {
				if (ffjson[i] == '"' && ffjson[i - 1] != '\\') {
					size = i - strNail;
					val.string = (char*) malloc(size + 1);
					memcpy(val.string, ffjson.c_str() + strNail,
							size);
					val.string[size] = '\0';
					i++;
					break;
				} else {
					i++;
				}
			}
			break;
		} else if (ffjson[i] == '<') {
			i++;
			int xmlNail = i;
			string xmlTag;
			int length = -1;
			bool tagset = false;
			while (ffjson[i] != '>' && i < j) {
				if (ffjson[i] == ' ') {
					tagset = true;
					if (ffjson[i + 1] == 'l' &&
							ffjson[i + 2] == 'e' &&
							ffjson[i + 3] == 'n' &&
							ffjson[i + 4] == 'g' &&
							ffjson[i + 5] == 't' &&
							ffjson[i + 6] == 'h') {
						i += 7;
						while (ffjson[i] != '=' && i < j) {
							i++;
						}
						i++;
						while (ffjson[i] != '"' && i < j) {
							i++;
						}
						i++;
						string lengthstr;
						while (ffjson[i] != '"' && i < j) {
							lengthstr += ffjson[i];
							i++;
						}
						length = stoi(lengthstr);
					}
				} else if (!tagset) {
					xmlTag += ffjson[i];
				}
				i++;
			}
			setType(XML);
			i++;
			xmlNail = i;
			if (length>-1 && length < (j - i)) {
				i += length;
			}
			while (i < j) {
				if (ffjson[i] == '<' &&
						ffjson[i + 1] == '/') {
					if (xmlTag.compare(ffjson.substr(i + 2, xmlTag.length()))
							== 0 && ffjson[i + 2 + xmlTag.length()] == '>') {
						size = i - xmlNail;
						val.string = (char*) malloc(size);
						memcpy(val.string, ffjson.c_str() + xmlNail,
								size);
						i += 3 + xmlTag.length();
						break;
					}
				}
				i++;
			}
			break;
		} else if (ffjson[i] == 't' &&
				ffjson[i + 1] == 'r' &&
				ffjson[i + 2] == 'u' &&
				ffjson[i + 3] == 'e') {
			setType(BOOL);
			val.boolean = true;
			i += 4;
			break;
		} else if (ffjson[i] == 'f' &&
				ffjson[i + 1] == 'a' &&
				ffjson[i + 2] == 'l' &&
				ffjson[i + 3] == 's' &&
				ffjson[i + 4] == 'e') {
			setType(BOOL);
			val.boolean = true;
			i += 5;
			break;
		} else if ((ffjson[i] >= '0' && ffjson[i] <= '9') || ffjson[i] == '-' ||
				ffjson[i] == '+') {
			int numNail = i;
			i++;
			while ((ffjson[i] >= '0' && ffjson[i] <= '9') || ffjson[i] == '.')
				i++;
			size = i - numNail;
			string num = ffjson.substr(numNail, i - numNail);
			val.number = stod(num);
			setType(OBJ_TYPE::NUMBER);
			break;
		} else if (ffjson[i] == '?') {
			setQType(QUERY);
			i++;
			break;
		} else if (ffjson[i] == 'd' &&
				ffjson[i + 1] == 'e' &&
				ffjson[i + 2] == 'l' &&
				ffjson[i + 3] == 'e' &&
				ffjson[i + 4] == 't' &&
				ffjson[i + 5] == 'e') {
			setQType(DELETE);
			i += 6;
			break;
		} else if (ffjson[i] == 'n' &&
				ffjson[i + 1] == 'u' &&
				ffjson[i + 2] == 'l' &&
				ffjson[i + 3] == 'l') {
			setType(NUL);
			i += 4;
			break;
		} else if (ffjson[i] == ',' || ffjson[i] == '}' || ffjson[i] == ']') {
			setType(NUL);
			size = 0;
			break;
		}
		i++;
	}
	if (ci != NULL)*ci = i;
}

FFJSON::~FFJSON() {
	freeObj();
}

void FFJSON::freeObj() {
	if (isType(OBJ_TYPE::OBJECT)) {
		map<string, FFJSON*>::iterator i;
		i = val.pairs->begin();
		while (i != val.pairs->end()) {
			delete i->second;
			i++;
		}
		delete val.pairs;
	} else if (isType(OBJ_TYPE::ARRAY)) {
		int i = val.array->size() - 1;
		while (i >= 0) {
			delete (*val.array)[i];
			i--;
		}
		delete val.array;
	} else if (isType(OBJ_TYPE::STRING) || isType(OBJ_TYPE::XML)) {
		free(val.string);
	}
}

void FFJSON::trimWhites(string & s) {
	int i = 0;
	int j = s.length() - 1;
	while (s[i] == ' ' || s[i] == '\n' || s[i] == '\t' || s[i] == '\r') {
		i++;
	}
	while (s[j] == ' ' || s[j] == '\n' || s[j] == '\t' || s[j] == '\r') {
		j--;
	}
	j++;
	s = s.substr(i, j - i);
}

void FFJSON::trimQuotes(string & s) {
	int i = 0;
	int j = s.length() - 1;
	if (s[0] == '"') {
		i++;
	}
	if (s[j] == '"') {
		j--;
	}
	j++;
	s = s.substr(i, j - i);
}

FFJSON::OBJ_TYPE FFJSON::objectType(string ffjson) {
	if (ffjson[0] == '{' && ffjson[ffjson.length() - 1] == '}') {
		return OBJ_TYPE::OBJECT;
	} else if (ffjson[0] == '"' && ffjson[ffjson.length() - 1] == '"') {
		return OBJ_TYPE::STRING;
	} else if (ffjson[0] == '[' && ffjson[ffjson.length() - 1] == ']') {
		return OBJ_TYPE::ARRAY;
	} else if (ffjson.compare("true") == 0 || ffjson.compare("false") == 0) {
		return OBJ_TYPE::BOOL;
	} else {
		return OBJ_TYPE::UNDEFINED;
	}
}

FFJSON & FFJSON::operator[](const char* prop) {
	return (*this)[string(prop)];
}

FFJSON & FFJSON::operator[](string prop) {
	if (isType(UNDEFINED)) {
		type = OBJECT;
		val.pairs = new map<string, FFJSON*>();
		size = 0;
	}
	if (isType(OBJ_TYPE::OBJECT)) {
		if ((*val.pairs).find(prop) != (*val.pairs).end()) {
			if ((*val.pairs)[prop] != NULL) {
				return *((*val.pairs)[prop]);
			} else {
				throw Exception("NULL");
			}
		} else {
			size++;
			return *((*val.pairs)[prop] = new FFJSON());
		}
	} else {
		throw Exception("NON OBJECT TYPE");
	}
}

FFJSON & FFJSON::operator[](int index) {
	if (isType(OBJ_TYPE::ARRAY)) {
		if ((*val.array).size() > index) {
			if ((*val.array)[index] == NULL) {
				(*val.array)[index] = new FFJSON(UNDEFINED);
			}
			return *((*val.array)[index]);
		} else if (index == size) {
			if ((*this)[size - 1].isType(UNDEFINED)) {
				return (*this)[size - 1];
			} else {
				FFJSON* f = new FFJSON();
				val.array->push_back(f);
				size++;
				return *f;
			}
		} else {
			throw Exception("NULL");
		}
	} else {
		throw Exception("NON ARRAY TYPE");
	}
};

/**
 * converts FFJSON object to json string
 * @param encode_to_base64 if true then the binary data is base64 encoded
 * @return json string of this FFJSON object
 */
string FFJSON::stringify(bool json) {
	if (isType(OBJ_TYPE::STRING)) {
		return ("\"" + string(val.string, size) + "\"");
	} else if (isType(OBJ_TYPE::NUMBER)) {
		return to_string(val.number);
	} else if (isType(OBJ_TYPE::XML)) {
		if (isType(B64ENCODE)) {
			int output_length = 0;
			char * b64_char = base64_encode((const unsigned char*) val.string,
					size, (size_t*) & output_length);
			string b64_str(b64_char, output_length);
			free(b64_char);
			return ("\"" + b64_str + "\"");
		} else {
			return ("<xml length=\"" + to_string(size) + "\">" +
					string(val.string, size) + "</xml>");
		}
	} else if (isType(OBJ_TYPE::BOOL)) {
		if (val.boolean) {
			return ("true");
		} else {
			return ("false");
		}
	} else if (isType(OBJ_TYPE::OBJECT)) {
		string ffs;
		map<string, FFJSON*>& objmap = *(val.pairs);
		ffs = "{";
		map<string, FFJSON*>::iterator i;
		i = objmap.begin();
		while (i != objmap.end()) {
			int t = i->second ? i->second->type : NUL;
			if (t != UNDEFINED) {
				if (t != NUL) {
					if (isType(B64ENCODE))i->second->setType(B64ENCODE);
					if ((isType(B64ENCODE_CHILDREN))&&!isType(B64ENCODE_STOP))
						i->second->type |= B64ENCODE_CHILDREN;
				}
				ffs.append("\"" + i->first + "\":");
				if (t != NUL) {
					ffs.append(i->second->stringify(json));
				} else if (json) {
					ffs.append("null");
				}
			}
			if (++i != objmap.end())if (t != UNDEFINED)ffs.append(",");
		}
		ffs += '}';
		return ffs;
	} else if (isType(OBJ_TYPE::ARRAY)) {
		string ffs;
		vector<FFJSON*>& objarr = *(val.array);
		ffs = "[";
		int i = 0;
		while (i < objarr.size()) {
			int t = objarr[i] ? objarr[i]->type : NUL;
			if (t == NUL) {
				if (json) {
					ffs.append("null");
				}
			} else if (t != UNDEFINED) {
				if (isType(B64ENCODE))objarr[i]->setType(B64ENCODE);
				if ((isType(B64ENCODE_CHILDREN))&&!isType(B64ENCODE_STOP))
					objarr[i]->setType(B64ENCODE_CHILDREN);
				ffs.append(objarr[i]->stringify(json));
			}
			if (++i != objarr.size()) {
				if (objarr[i] && objarr[i]->type != UNDEFINED) {
					ffs.append(",");
				} else {
					ffs.append(",");
				}
			}
		}
		ffs += ']';
		return ffs;
	} else if (!isQType(NONE)) {
		if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DELETE)) {
			return "delete";
		} else {
			return "";
		}
	} else {
	}
}

string FFJSON::prettyString(unsigned int indent) {
	if (isType(OBJ_TYPE::STRING)) {
		string ps = "\"";
		int stringNail = 0;
		int i = 0;
		for (i = 0; i < size; i++) {
			if (val.string[i] == '\n') {
				ps.append(val.string, stringNail, i + 1 - stringNail);
				ps.append('\t', indent);
				stringNail = i + 1;
			}
		}
		ps.append(val.string, stringNail, i - stringNail);
		ps += "\"";
		return ps;
	} else if (isType(OBJ_TYPE::NUMBER)) {
		return to_string(val.number);
	} else if (isType(OBJ_TYPE::XML)) {
		if (isType(B64ENCODE)) {
			int output_length = 0;
			char * b64_char = base64_encode(
					(const unsigned char*) val.string,
					size, (size_t*) & output_length);
			string b64_str(b64_char, output_length);
			free(b64_char);
			return ("\"" + b64_str + "\"");
		} else {
			return ("<xml length = \"" + to_string(size) + "\" >" +
					string(val.string, size) + "</xml>");
		}
	} else if (isType(OBJ_TYPE::BOOL)) {
		if (val.boolean) {
			return ("true");
		} else {
			return ("false");
		}
	} else if (isType(OBJ_TYPE::OBJECT)) {
		map<string, FFJSON*>& objmap = *(val.pairs);
		string ps = "{\n";
		map<string, FFJSON*>::iterator i;
		i = objmap.begin();
		while (i != objmap.end()) {
			uint8_t t = i->second ? i->second->type : NUL;
			if (t != UNDEFINED && t != NUL) {
				if (isType(B64ENCODE))i->second->setType(B64ENCODE);
				if ((isType(B64ENCODE_CHILDREN))&&!isType(B64ENCODE_STOP))
					i->second->type |= B64ENCODE_CHILDREN;
				ps.append(indent + 1, '\t');
				ps.append("\"" + i->first + "\": ");
				ps.append(i->second->prettyString(indent + 1));
			} else if (t == NUL) {
				ps.append(indent + 1, '\t');
				ps.append("\"" + i->first + "\": ");
			}
			if (++i != objmap.end()) {
				if (t != UNDEFINED)ps.append(",\n");
			} else {
				ps.append("\n");
			}
		}
		ps.append(indent, '\t');
		ps.append("}");
		return ps;
	} else if (isType(OBJ_TYPE::ARRAY)) {
		vector<FFJSON*>& objarr = *(val.array);
		string ps = "[\n";
		int i = 0;
		while (i < objarr.size()) {
			uint8_t t = objarr[i] ? objarr[i]->type : NUL;
			if (t != UNDEFINED && t != NUL) {
				if (isType(B64ENCODE))objarr[i]->setType(B64ENCODE);
				if ((isType(B64ENCODE_CHILDREN))&&!isType(B64ENCODE_STOP))
					objarr[i]->setType(B64ENCODE_CHILDREN);
				ps.append(indent + 1, '\t');
				ps.append(objarr[i]->prettyString(indent + 1));
			} else if (t == NUL) {
				ps.append(indent + 1, '\t');
			}
			if (++i != objarr.size()) {
				/*if (objarr[i] && objarr[i]->type != UNDEFINED) {
					ps.append(",\n");
				} else {
					ps.append(",\n");
				}*/
				ps.append(",\n");
			} else {
				ps.append("\n");
			}
		}
		ps.append(indent, '\t');
		ps.append("]");
		return ps;
	} else if (!isQType(NONE)) {
		if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DELETE)) {
			return "delete";
		} else {
			return "";
		}
	} else {
		return "";
	}
}

/**
 */
FFJSON::operator const char*() {
	return val.string;
}

FFJSON::operator double() {
	return val.number;
}

FFJSON::operator float() {
	return (float) val.number;
}

FFJSON::operator bool() {
	if (type != BOOL && type != UNDEFINED && type != NUL) {
		return true;
	} else if (type == BOOL) {
		return val.boolean;
	} else {
		return false;
	}
}

FFJSON::operator int() {
	return (int) val.number;
}

FFJSON::operator unsigned int() {
	return (unsigned int) val.number;
}

FFJSON& FFJSON::operator=(const string& s) {
	freeObj();
	int i = 0;
	int j = s.length();
	if (s[0] == '<') {
		i++;
		int xmlNail = i;
		string xmlTag;
		int length = -1;
		bool tagset = false;
		while (s[i] != '>' && i < j) {
			if (s[i] == ' ') {
				tagset = true;
				if (s[i + 1] == 'l' &&
						s[i + 2] == 'e' &&
						s[i + 3] == 'n' &&
						s[i + 4] == 'g' &&
						s[i + 5] == 't' &&
						s[i + 6] == 'h') {
					i += 7;
					while (s[i] != '=' && i < j) {
						i++;
					}
					i++;
					while (s[i] != '"' && i < j) {
						i++;
					}
					i++;
					string lengthstr;
					while (s[i] != '"' && i < j) {
						lengthstr += s[i];
						i++;
					}
					length = stoi(lengthstr);
				}
			} else if (!tagset) {
				xmlTag += s[i];
			}
			i++;
		}
		setType(XML);
		i++;
		xmlNail = i;
		if (length>-1 && length < (j - i)) {
			i += length;
		}
		while (i < j) {
			if (s[i] == '<' &&
					s[i + 1] == '/') {
				if (xmlTag.compare(s.substr(i + 2, xmlTag.length()))
						== 0 && s[i + 2 + xmlTag.length()] == '>') {
					size = i - xmlNail;
					val.string = (char*) malloc(size + 1);
					memcpy(val.string, s.c_str() + xmlNail,
							size);
					val.string[size] = '\0';
					i += 3 + xmlTag.length();
					break;
				}
			}
			i++;
		}
	} else {
		setType(STRING);
		size = s.length();
		val.string = (char*) malloc(size + 1);
		memcpy(val.string, s.c_str(),
				size);
		val.string[size] = '\0';
	}
}

FFJSON& FFJSON::operator=(const int& i) {
	freeObj();
	setType(NUMBER);
	val.number = i;
}

FFJSON& FFJSON::operator=(const FFJSON& f) {
	freeObj();
	copy(f);
}

FFJSON& FFJSON::operator=(const FFJSON* f) {

}

FFJSON& FFJSON::operator=(const double& d) {
	freeObj();
	setType(NUMBER);
	val.number = d;
}

FFJSON& FFJSON::operator=(const float& f) {
	freeObj();
	setType(NUMBER);
	val.number = f;
}

FFJSON& FFJSON::operator=(const long& l) {
	freeObj();
	setType(NUMBER);
	val.number = l;
}

FFJSON& FFJSON::operator=(const short& s) {
	freeObj();
	setType(NUMBER);
	val.number = s;
}

FFJSON& FFJSON::operator=(const unsigned int& i) {
	freeObj();
	setType(NUMBER);
	val.number = i;
}

void FFJSON::trim() {
	if (isType(OBJECT)) {
		map<string, FFJSON*>::iterator i;
		i = val.pairs->begin();
		while (i != val.pairs->end()) {
			if (((*i->second).isType(UNDEFINED)&&!(*i->second).isQType(NONE)) || (*i->second).isType(NUL)) {
				delete i->second;
				map<string, FFJSON*>::iterator j = i;
				i++;
				val.pairs->erase(j);
				size--;
			} else {
				i++;
			}
		}
	} else if (isType(ARRAY)) {
		if ((*this)[size - 1].isType(UNDEFINED)) {
			delete (*val.array)[size - 1];
			val.array->pop_back();
			size--;
		}
		int i = 0;
		while (i < val.array->size()) {
			if ((*val.array)[i] != NULL) {
				if (((*val.array)[i]->isType(UNDEFINED)&&
						!(*val.array)[i]->isQType(NONE)) ||
						(*val.array)[i]->isType(NUL)) {
					delete (*val.array)[i];
					(*val.array)[i] = NULL;
				}
			}
			i++;
		}
	}
}

string FFJSON::queryString() {
	if (isType(OBJ_TYPE::STRING)) {
		if (isQType(SET)) {
			return ("\"" + string(val.string, size) + "\"");
		} else if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DELETE)) {
			return "delete";
		} else {
			return "";
		}
	} else if (isType(OBJ_TYPE::NUMBER)) {
		if (isQType(SET)) {
			return to_string(val.number);
		} else if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DELETE)) {
			return "delete";
		} else {
			return "";
		}
	} else if (isType(OBJ_TYPE::XML)) {
		if (isQType(SET)) {
			if (isType(B64ENCODE)) {
				int output_length = 0;
				char * b64_char = base64_encode((const unsigned char*) val.string,
						size, (size_t*) & output_length);
				string b64_str(b64_char, output_length);
				free(b64_char);
				return ("\"" + b64_str + "\"");
			} else {
				return ("<xml length=\"" + to_string(size) + "\">" +
						string(val.string, size) + "</xml>");
			}
		} else if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DELETE)) {
			return "delete";
		} else {
			return "";
		}
	} else if (isType(OBJ_TYPE::BOOL)) {
		if (isQType(SET)) {
			if (val.boolean) {
				return ("true");
			} else {
				return ("false");
			}
		} else if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DELETE)) {
			return "delete";
		} else {
			return "";
		}
	} else if (isType(OBJ_TYPE::OBJECT)) {
		if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DELETE)) {
			return "delete";
		} else {
			string ffs;
			map<string, FFJSON*>& objmap = *(val.pairs);
			ffs = "{";
			map<string, FFJSON*>::iterator i;
			i = objmap.begin();
			bool matter = false;
			while (i != objmap.end()) {
				string ffjsonStr;
				uint8_t t = (i->second != NULL) ? i->second->type : NUL;
				if (t != UNDEFINED || (t != NUL&&!i->second->isQType(NONE))) {
					if (t != NUL) {
						if (isType(B64ENCODE))i->second->setType(B64ENCODE);
						if ((isType(B64ENCODE_CHILDREN))&&!isType(B64ENCODE_STOP))
							i->second->type |= B64ENCODE_CHILDREN;
						ffjsonStr = i->second->queryString();
					}
					if (ffjsonStr.length() > 0) {
						if (matter)ffs.append(",");
						ffs.append("\"" + i->first + "\":");
						ffs.append(ffjsonStr);
						matter = true;
					}
				}
				i++;
			}
			if (ffs.length() == 1) {
				ffs = "";
			} else {
				ffs += '}';
			}
			return ffs;
		}
	} else if (isType(OBJ_TYPE::ARRAY)) {
		if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DELETE)) {
			return "delete";
		} else {
			string ffs;
			vector<FFJSON*>& objarr = *(val.array);
			ffs = "[";
			bool matter = false;
			int i = 0;
			string ffjsonstr;
			bool firstTime = false;
			while (i < objarr.size()) {
				unsigned int t = objarr[i] != NULL ? objarr[i]->type : NUL;
				if (t != UNDEFINED || (t != NUL&&!objarr[i]->isQType(NONE))) {
					if (t != NUL) {
						if (isType(B64ENCODE))objarr[i]->setType(B64ENCODE);
						if ((isType(B64ENCODE_CHILDREN))&&!isType(B64ENCODE_STOP))
							objarr[i]->setType(B64ENCODE_CHILDREN);
						ffjsonstr = objarr[i]->queryString();
					} else {
						ffjsonstr = "";
					}
					if (firstTime)ffs += ',';
					firstTime = true;
					if (ffjsonstr.length() > 0) {
						ffs.append(ffjsonstr);
						matter = true;
					}
				}
				i++;
			}
			if (matter) {
				ffs += ']';
			} else {
				ffs = "";
			};
			return ffs;
		}
	} else if (!isQType(NONE)) {
		if (isQType(QUERY)) {
			return "?";
		} else if (isQType(DELETE)) {
			return "delete";
		} else {
			return "";
		}
	} else {
		return "";
	}
}

FFJSON* FFJSON::answerObject(FFJSON* queryObject) {
	FFJSON* ao = NULL;
	if (queryObject->isType(this->type) || queryObject->isQType(QUERY) ||
			queryObject->isQType(DELETE)) {
		if (queryObject->isQType(DELETE)) {
			this->freeObj();
			type = NUL;
		} else if (queryObject->isQType(QUERY)) {
			ao = new FFJSON(*this);
		} else if (queryObject->isType(OBJECT)) {
			map<string, FFJSON*>::iterator i = queryObject->val.pairs->begin();
			while (i != queryObject->val.pairs->end()) {
				map<string, FFJSON*>::iterator j;
				if ((j = (*this).val.pairs->find(i->first)) != this->val.pairs->end()) {
					FFJSON* lao = j->second->answerObject(i->second);
					if (lao != NULL) {
						if (ao == NULL)ao = new FFJSON(OBJECT);
						(*(*ao).val.pairs)[i->first] = lao;
					}
				} else {
					/*FFJSON* nao = new FFJSON(*i->second);
					if (!nao->isType(UNDEFINED)) {
						(*this).val.pairs[i->first] = nao;
					}*/
				}
				i++;
			}
		} else if (queryObject->isType(ARRAY)) {
			if (queryObject->size == size) {
				int i = 0;
				bool matter = false;
				ao = new FFJSON(ARRAY);
				while (i < queryObject->size) {
					if ((*queryObject->val.array)[i] != NULL) {
						FFJSON* ffo = NULL;
						ffo = (*this->val.array)[i]->answerObject
								((*queryObject->val.array)[i]);
						if (ffo) {
							ao->val.array->push_back(ffo);
							matter = true;
						} else {
							ao->val.array->push_back(NULL);
						}
					} else {
						ao->val.array->push_back(NULL);
					}
					i++;
				}
				if (!matter) {
					delete ao;
					ao = NULL;
				}
			}
		} else if (queryObject->isType(STRING) ||
				queryObject->isType(XML) ||
				queryObject->isType(BOOL) ||
				queryObject->isType(NUMBER)) {
			freeObj();
			this->copy(*queryObject);
		} else {
			ao = NULL;
		}
	}
	return ao;
}

bool FFJSON::isType(uint8_t t) const {
	return (t == type);
}

void FFJSON::setType(uint8_t t) {
	type = t;
}

uint8_t FFJSON::getType() const {
	return type;
}

bool FFJSON::isQType(uint8_t t) const {
	return (t == qtype);
}

void FFJSON::setQType(uint8_t t) {
	qtype = t;
}

uint8_t FFJSON::getQType() const {
	return qtype;
}

bool FFJSON::isEFlagSet(uint8_t t) const {
	return (t & etype == t);
}

uint8_t FFJSON::getEFlags() const {
	return this->etype;
}

void FFJSON::setEFlag(uint8_t t) {
	etype |= t;
}

void FFJSON::clearEFlag(uint8_t t) {
	etype &= ~t;
}

void FFJSON::erase(string name) {
	if (isType(OBJECT)) {
		delete (*val.pairs)[name];
		val.pairs->erase(name);
	}
}

void FFJSON::erase(int index) {
	if (isType(ARRAY)) {
		if (index < size) {
			delete (*val.array)[index];
			(*val.array)[index] = NULL;
		}
	}
}

void FFJSON::erase(FFJSON* value) {
	if (isType(OBJECT)) {
		map<string, FFJSON*>::iterator i = val.pairs->begin();
		while (i != val.pairs->end()) {
			if (i->second == value) {
				delete i->second;
				val.pairs->erase(i);
				break;
			}
			i++;
		}
	} else if (isType(ARRAY)) {
		int i = 0;
		while (i < size) {
			if ((*val.array)[i] == value) {
				delete (*val.array)[i];
				(*val.array)[i] = NULL;
				break;
			}
			i++;
		}
	}
}

FFJSON::Iterator FFJSON::begin() {
	Iterator i(*this);
	return i;
}

FFJSON::Iterator FFJSON::end() {
	Iterator i(*this, true);
	return i;
}

FFJSON::Iterator::Iterator() {
	type = NUL;
}

FFJSON::Iterator::Iterator(const Iterator& orig) {
	copy(orig);
}

FFJSON::Iterator::Iterator(const FFJSON& orig, bool end) {
	init(orig, end);
}

FFJSON::Iterator::~Iterator() {
	if (type == OBJECT) {
		delete ui.pi;
	} else if (type == ARRAY) {
		delete ui.ai;
	}
}

void FFJSON::Iterator::copy(const Iterator& i) {
	type = i.type;
	if (type == OBJECT) {
		ui.pi = new map<string, FFJSON*>::iterator();
		(*ui.pi) = *i.ui.pi;
	} else if (type == ARRAY) {
		ui.ai = new vector<FFJSON*>::iterator();
		(*ui.ai) = *i.ui.ai;
	}
}

void FFJSON::Iterator::init(const FFJSON& orig, bool end) {
	if (orig.isType(ARRAY)) {
		type = ARRAY;
		ui.ai = new vector<FFJSON*>::iterator();
		(*ui.ai) = end ? orig.val.array->end() : orig.val.array->begin();
	} else if (orig.isType(OBJECT)) {
		type = OBJECT;
		ui.pi = new map<string, FFJSON*>::iterator();
		(*ui.pi) = end ? orig.val.pairs->end() : orig.val.pairs->begin();
	} else {
		type = NUL;
		ui.ai = NULL;
	}
}

FFJSON::Iterator& FFJSON::Iterator::operator=(const Iterator& i) {
	copy(i);
}

FFJSON& FFJSON::Iterator::operator*() {
	if (type == OBJECT) {
		return *((*ui.pi)->second);
	} else if (type == ARRAY) {
		return **(*ui.ai);
	}
}

FFJSON* FFJSON::Iterator::operator->() {
	if (type == OBJECT) {
		return ((*ui.pi)->second);
	} else if (type == ARRAY) {
		return &(**(*ui.ai));
	}
}

FFJSON::Iterator& FFJSON::Iterator::operator++() {
	if (type == OBJECT) {
		(*ui.pi)++;
	} else if (type == ARRAY) {
		(*ui.ai)++;
	}
	return *this;
}

FFJSON::Iterator FFJSON::Iterator::operator++(int) {
	FFJSON::Iterator tmp(*this);
	operator++();
	return tmp;
}

FFJSON::Iterator& FFJSON::Iterator::operator--() {
	if (type == OBJECT) {
		(*ui.pi)--;
	} else if (type == ARRAY) {
		(*ui.ai)--;
	}
	return *this;
}

FFJSON::Iterator FFJSON::Iterator::operator--(int) {
	FFJSON::Iterator tmp(*this);
	operator++();
	return tmp;
}

bool FFJSON::Iterator::operator==(const Iterator& i) {
	if (type == i.type) {
		if (type == OBJECT) {
			return ((*ui.pi) == (*i.ui.pi));
		} else if (type == ARRAY) {
			return (*ui.ai == *i.ui.ai);
		} else if (type == NUL) {
			return true;
		}
	}
	return false;
}

bool FFJSON::Iterator::operator!=(const Iterator& i) {
	return !operator==(i);
}

FFJSON::Iterator::operator const char*() {
	if (type == OBJECT) {
		return (*ui.pi)->first.c_str();
	}
	return NULL;
}
