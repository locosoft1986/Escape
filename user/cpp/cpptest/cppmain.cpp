/**
 * $Id: ctmain.c 202 2009-04-11 16:07:24Z nasmussen $
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <esc/common.h>
#include <esc/fileio.h>
#include <stdlib.h>
#include <esc/vector.h>
#include <esc/stream.h>
#include <esc/string.h>

using namespace esc;

class my {
public:
	my() {
		printf("Constructor for %p...\n",this);
	};
	~my() {
		printf("Destructor for %p...\n",this);
	};

	void doIt();
};

my x;

void my::doIt() {
	printf("Ich bins\n");
}

int main(void) {
	char str[10];
	char buffer[1024];
	char *test = "ein test";
	StringStream s(str,sizeof(str));
	FileStream f("file:/file.txt",FileStream::READ);
	s << 'a' << 'b';
	s.write(test,strlen(test));
	printf("%s\n",str);
	/*char c;
	esc::in >> c;
	esc::out << c;*/
	f.read(buffer,sizeof(buffer));
	out << buffer << endl;

	out << "test" << endl;
	out << -1234 << endl;
	out << 'a' << 'b' << 'c' << endl;
	out.format("das=%d, bin=%x, ich=%s\n",-193,0xABC,"test");
	out.format("out.pos=%d\n",out.getPos());

	String mystr = "ich";
	mystr += "test";
	mystr += 'a';
	mystr += String("abc");
	String abc("test");
	out << "str=" << mystr << ", length=" << mystr.length() << endl;
	out << abc << endl;

	out << "offset of a=" << mystr.find('a') << endl;
	out << "offset of test=" << mystr.find("test") << endl;

	Vector<s32> v;
	for(s32 i = 0; i < 20; i++)
		v.add(i);
	Vector<s32> v2 = v;
	v.insert(4,1024);
	v.insert(0,123);
	v.insert(v.size(),0);
	v.insert(v.size() - 1,456);

	for(u32 i = 0; i < v.size(); i++) {
		out << i << ": " << v[i] << endl;
	}
	for(u32 i = 0; i < v2.size(); i++) {
		out << i << ": " << v2[i] << endl;
	}

	/*unsigned int a = 0;
	a++;
	printf("a=%d\n",a);

	x.doIt();
	y->doIt();

	my *m = new my();
	m->doIt();
	delete m;
	delete y;*/
	return EXIT_SUCCESS;
}
