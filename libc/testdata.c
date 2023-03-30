/*
 * Phoenix-RTOS
 *
 * libc-test-data
 *
 * Helpers regarding data for libc testing puproses
 *
 * Copyright 2023 Phoenix Systems
 * Author: Damian Loewnau, Mateusz Bloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include <limits.h>
#include <stddef.h>

char *testdata_createCharStr(int size)
{
	char *dataStr = calloc(size, 1);
	int i;

	if (dataStr == NULL) {
		return NULL;
	}

	for (i = 0; i < size - 1; i++) {
		dataStr[i] = i % (UCHAR_MAX + 1);
		/* double one to prevent setting NUL term */
		if (dataStr[i] == 0) {
			dataStr[i] = 1;
		}
	}

	return dataStr;
}

/* Test Data */
const char testdata_hugeStr[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas eleifend elementum tellu"
								"s, ut consequat tellus porttitor sed. In nec condimentum sem. Suspendisse nec lacus sagitt"
								"is, ornare nulla eu, pulvinar odio. Nullam pellentesque sapien congue dignissim sollicitud"
								"in. Quisque efficitur tincidunt rhoncus. Quisque a iaculis libero. Donec tincidunt congue "
								"various. Maecenas tristique, justo a commodo pulvinar, lectus mi malesuada massa, ac laore"
								"et neque purus convallis turpis. Aenean pulvinar volutpat enim at mattis. Suspendisse pote"
								"nti. Proin cursus, libero sit amet feugiat pretium, neque elit tristique libero, eleifend "
								"rutrum turpis sem vel est. Donec vestibulum augue id magna posuere efficitur. Donec pulvin"
								"ar lacinia imperdiet. Morbi iaculis et erat ut consectetur. Maecenas quam purus, euismod a"
								" aliquam vitae, sollicitudin quis eros. Donec pulvinar, metus vel pharetra lobortis, dolor"
								" quam various massa, ac euismod nulla sem ut enim. Name in magna quis massa tristique laor"
								" eet. Pellentesque sem sapien, fringilla sit amet sem quis, semper volutpat mauris. Aenean"
								" gravida nibh a aliquam vehicula. Proin hendrerit urna facilisis aliquam maximus. Sed male"
								"suada porttitor placerat. Cras a metus id arcu gravida ultricies. Etiam hendrerit justo ut"
								" venenatis hendrerit. Etiam vulputate erat vel ligula faucibus, quis posuere purus efficit"
								"ur. Phasellus aliquet eros id nunc vulputate pharetra. Maecenas non euismod diam. Duis at "
								"sagittis tortor, id aliquam eros. Ut sed ligula velit. Donec vel magna ullamcorper, hendre"
								"rit elit vel, porttitor dui. Sed sapien tortor, vehicula euismod erat bibendum, dignissim "
								"viverra nibh. Vivamus et ex arcu. Name pharetra risus cursus mi interdum ullamcorper. Proi"
								"n vel urna scelerisque est convallis lacinia. Mauris libero enim, consequat quis justo id,"
								" placerat ultricies nisi. Aliquam posuere tortor ut ipsum commodo, vitae efficitur eros ul"
								"trices. Aliquam gravida sodales quam, at sodales leo elementum eget. Fusce fermentum metus"
								" in bibendum euismod. Morbi porta imperdiet justo a posuere. Curabitur id risus risus. Pra"
								"esent mollis blandit orci, eget semper ligula consectetur at. Phasellus vestibulum enim et"
								" diam dapibus, at suscipit libero pretium. Nulla facilisi. Fusce gravida dapibus tincidunt"
								". Sed non dapibus eros, non efficitur nunc. Phasellus augue quam, rhoncus a sem a, blandit"
								" mollis nunc. Donec pharetra, orci quis euismod porttitor, orci nibh vulputate felis, eu d"
								"ignissim eros mauris id dolor. Nullam dictum venenatis leo sed accumsan. Pellentesque vari"
								"ous diam felis, sed feugiat nisi hendrerit volutpat. Ut faucibus efficitur sollicitudin. N"
								"ulla efficitur massa dignissim sem dignissim, vel dignissim sem elementum. Nullam semper a"
								" metus ac molestie. Aenean tempus risus lectus, eu semper ipsum lobortis sit amet. Etiam v"
								"el semper enim, vel lobortis augue. Quisque vehicula quam consectetur leo porttitor, quis "
								"cursus dolor posuere. Vestibulum semper ut orci vitae venenatis. Cras commodo mauris eu li"
								"gula auctor sagittis. Sed bibendum orci vitae interdum elementum. Praesent vehicula orci q"
								"uis lectus accumsan ultrices. Proin vehicula mollis ipsum nec tristique. Nulla non lacus n"
								"ec lacus luctus ullamcorper. Fusce libero quam, molestie non nisi eget, fermentum vestibul"
								"um urna. In ante ligula, tempor in neque id, faucibus tincidunt tortor. Class aptent tacit"
								"i sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Nullam suscipit "
								"rhoncus consequat. Maecenas fermentum molestie est non finibus. Cras auctor purus ac tellu"
								"s euismod commodo. In faucibus dapibus hendrerit. Nulla dapibus quis ligula eget efficitur"
								". Donec tempor quis est a fermentum. Maecenas ut dui elementum, lobortis nibh non, euismod"
								" nunc. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc ex nisl, vestibulum q"
								"uis cursus sed, maximus vitae est. Quisque lobortis tellus metus, ut rhoncus ex dignissim "
								"vitae. Pellentesque inhabitant morbi tristique senectus et netus et malesuada fames ac tur"
								"pis egestas. Suspendisse enim nisl, egestas nec posuere a, mattis ac nisi. Duis lobortis b"
								"landit bibendum. Donec ac lorem sed ante fringilla. Lorem ipsum dolor sit amet, consectetu"
								"r adipiscing elit. Lorem ipsum dolor sit amet, consectetur adipiscing elit. ";

const size_t testdata_hugeSize = sizeof(testdata_hugeStr);
