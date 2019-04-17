int uint32toByteArray(uint32_t x, uint8_t* bytes);

int uint32_31toByteArray(uint32_t x, uint8_t* bytes);

int uint32_24toByteArray(uint32_t x, uint8_t* bytes);


int uint16toByteArray(uint16_t x, uint8_t* bytes);

//byte to numbers
uint32_t bytesToUint32(uint8_t* bytes);

uint16_t bytesToUint16(uint8_t* bytes);

uint32_t bytesToUint32_24(uint8_t* bytes);

uint32_t bytesToUint32_31(uint8_t* bytes);


int appendByteArrays(uint8_t* dest, uint8_t* array1, uint8_t* array2, int size1, int size2);