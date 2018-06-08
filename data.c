#include "chess.h"
/* *INDENT-OFF* */
int scale = 10;
FILE *input_stream;
FILE *book_file;
FILE *books_file;
FILE *normal_bs_file;
FILE *computer_bs_file;
FILE *history_file;
FILE *log_file;
int done = 0;
BITBOARD burner[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
int burnc[10] = {40000, 20000, 10000, 4000, 2000, 800, 400, 200, 100, 0};
BITBOARD total_moves;
int allow_cores = 1;
int allow_memory = 1;
int last_mate_score;
char log_filename[256];
char history_filename[256];
int number_of_solutions;
int solutions[10];
int solution_type;
char cmd_buffer[4096];
char *args[512];
char buffer[4096];
int line_length = 80;
unsigned char convert_buff[8];
int nargs;
int ponder_value;
int move_actually_played;
int ponder_move;
int ponder_moves[220];
int num_ponder_moves;
int book_move;
int book_learn_eval[LEARN_INTERVAL];
int book_learn_depth[LEARN_INTERVAL];
int learn_positions_count = 0;
int learn_seekto[64];
BITBOARD learn_key[64];
int learn_nmoves[64];
int book_learn_nmoves;
int book_learn_seekto;
BITBOARD book_learn_key;
HASH_ENTRY *trans_ref;
PAWN_HASH_ENTRY *pawn_hash_table;
void * segments[MAX_BLOCKS + 32][2];
int nsegments = 0;
PATH last_pv;
int last_value;
int king_safety[16][16];
int mob_curve_r[48] = {
  -27,-23,-21,-19,-15,-10, -9, -8,
   -7, -6, -5, -4, -3, -2, -1,  0,
    1,  2,  3,  4,  5,  6,  7,  8,
    9, 10, 11, 12, 13, 14, 15, 16,
   17, 18, 19, 20, 21, 22, 23, 24,
   25, 26, 27, 28, 29, 30, 31, 32
};
signed char directions[64][64];
BITBOARD randoms[2][7][64] = {
  {
    { 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL },
    { 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0xd1fc122dd721044cULL,
      0xa4159629bd0ce70eULL, 0xab5da9e9ae24ad63ULL, 0x32e60a2983d1c843ULL,
      0x3c3cf99dabf131aaULL, 0xd83283085553e1fdULL, 0x180370f4abada20fULL,
      0xa7db417ed5cef0f6ULL, 0x8940b08b9b2fc0d4ULL, 0x852d84b34edc83d2ULL,
      0x068d4a5f2548652fULL, 0x35ce432f12163d2eULL, 0xc9ba66fee4843746ULL,
      0xabccc0992b67af9eULL, 0x217f1caa6a824b26ULL, 0x4a05addc1ea2e944ULL,
      0xfe2312497bf4c414ULL, 0x8495248ab305ee8fULL, 0xcb96c4247c24e036ULL,
      0xab76533d29e3c6eaULL, 0xc0944c15e3c09778ULL, 0x1053b4ccf6c024d6ULL,
      0x8d96dda010ba133aULL, 0x9f59cef04505da02ULL, 0x581ae15866c42214ULL,
      0x3a61654f9da998bfULL, 0x47efd3ec19fb73c0ULL, 0x2126b228fdb69cb5ULL,
      0xbb2ff9574df0d641ULL, 0x32b9d1ed571b84b8ULL, 0x4f688c6727828a1fULL,
      0x576784e75cc9d113ULL, 0x15e82e121ffd9115ULL, 0xbc0156dbcef2a7deULL,
      0x6365ce9628bd842eULL, 0x3a898fc4ece11b80ULL, 0xa6f7652af004d29fULL,
      0x5fa9ca7e22d71d72ULL, 0x5a06111a2088be1fULL, 0x05aae96e3384dfe4ULL,
      0xe697a8f517ab3d3fULL, 0x9d3c1d1302e84551ULL, 0x6e0b75ab4901a6c9ULL,
      0xd35c48fba6cf7eefULL, 0xc8ee1ca11e20a35eULL, 0x382637115387cc68ULL,
      0x8d499580f4544852ULL, 0x1e4a273c98576ebdULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL },
    { 0x2009a159d9509cb5ULL, 0x5fa8c13086419e55ULL, 0xea13669343bcd4b9ULL,
      0x3e47b851efbef3a4ULL, 0x6caa31dec6814cc6ULL, 0x992161e6ef2c2919ULL,
      0x8bed069c11856a55ULL, 0x5c73de210d683e98ULL, 0xa7185f2616bd83c1ULL,
      0xd7c04ef29cf3751eULL, 0x9f46f244eaf78bcfULL, 0x38f85c7bcda11a85ULL,
      0x22f1e6aabfe845aaULL, 0xa8aa67f9237dbb68ULL, 0x90e2e582b1c84a64ULL,
      0x5c685bafc31b3c9fULL, 0xc031fe468f6f6967ULL, 0x8d75aa382752602aULL,
      0x244475599eceffd5ULL, 0x0d0cfcd52bb265d4ULL, 0xda433164b4750dc1ULL,
      0x85dfeab10d5752d6ULL, 0x7225a1188a0376ebULL, 0xa8be7b123a102607ULL,
      0x603b914a4429875eULL, 0x399be2aedab54c3bULL, 0x58d916991d8a5cc4ULL,
      0x35b8f864e4685bdbULL, 0x46739c9baf790ac0ULL, 0xd1327ef186fb215aULL,
      0x140368fd88e26668ULL, 0xd38e499b69fa1e25ULL, 0xc96f5a7d211f3f9fULL,
      0x97655506a678b4bcULL, 0x7f8ddfb015f07d87ULL, 0xbf7bfed396840428ULL,
      0xfd2f0668eb41e684ULL, 0x24936e7db2759b2dULL, 0x267a311ebcd14f08ULL,
      0xf5a13e0e1d60b856ULL, 0x69f15803d26af16aULL, 0x82fac9552fa58953ULL,
      0xd71ea59c19c54beaULL, 0x62fdb302c66279d4ULL, 0xf0c1baa7e1513de5ULL,
      0x363454db33ee5ef4ULL, 0xab4045843a34f2e0ULL, 0x73f10c94a39100fdULL,
      0x0a86593ec58517e8ULL, 0xae96e568385585deULL, 0x608a57252ef6f020ULL,
      0x39cac56cc61f3368ULL, 0x44a6a43e32a682b4ULL, 0xa085261969416c01ULL,
      0xbb870177729e6283ULL, 0x1276bdc2fc1b7238ULL, 0x77bcdc0127c4da80ULL,
      0xbf16bb430520dde5ULL, 0x153a63a94e385bddULL, 0x95104c33771f4a98ULL,
      0xb0ff0dfa8ef47265ULL, 0xc48762efcfbadae4ULL, 0xf70e5b4dae84cfd0ULL,
      0x250cabf0859b323bULL },
    { 0xf71c3f6b2fd5891cULL, 0x5d5675218dd4da7eULL, 0xd8e7b9cd991893a1ULL,
      0xe4e1af2cf1bcb046ULL, 0xa762655491b63283ULL, 0xd8dc072d881ba73aULL,
      0xdb9cdd7d25570179ULL, 0x76618a18da1cfd9aULL, 0xe4ed6cee7fb62d0cULL,
      0xfebc55cf05ef2fbfULL, 0x9a9f5c4a59c51554ULL, 0xea91c3d8f98c3ecfULL,
      0x2f71c5493a5be25aULL, 0x8c4d65413c6bbfe6ULL, 0x9f06ed35f9a2eac6ULL,
      0x178831e02b8775b2ULL, 0x10a3d155d65cd6c1ULL, 0x4b7a69b028a0cd53ULL,
      0xd7b84fb5e0de02f0ULL, 0x9e266498ec93bb5cULL, 0xea0e4ad3f773090dULL,
      0x4cf4b1acc505eb02ULL, 0x11bea6e26eff78fdULL, 0x7986363d8e9c8b02ULL,
      0xff2ca02deb5af054ULL, 0x3ad2351100f322d5ULL, 0x24a21f70f0b14613ULL,
      0x475dbafc509421a2ULL, 0x9e78abd3cd79162cULL, 0xe492fd5185f74274ULL,
      0xc974fffbb9c5bcbdULL, 0x2fdc971bd1756a88ULL, 0x7229f20d9ed56e61ULL,
      0x724bb3259de1f22fULL, 0xebdba47c4a9c567aULL, 0xf93c634904fe151aULL,
      0x051bcfecd0485d42ULL, 0x94974a65ffaf78a4ULL, 0x1e2e5e3f8b50a25dULL,
      0x81b99d563b25e57aULL, 0xdae46bb9fceadb75ULL, 0xde35e8171244a7b5ULL,
      0xf4fc75ad58cca9f2ULL, 0x06396ddce8fd9c68ULL, 0x9acabf60af793fb6ULL,
      0x5329b615e95c5bdfULL, 0xd86f5fd82254a62fULL, 0xf0af6c56a32479b2ULL,
      0xca21baeeba56a815ULL, 0x0396c3624ca42a50ULL, 0xaccae9419d0f493fULL,
      0x6d148daab4ac1bb7ULL, 0xdbd78e8efc177ba2ULL, 0xcee72e2a4d16dcc6ULL,
      0x4590974f7d6ec962ULL, 0xd3b28408305b5764ULL, 0x20c201d44df1eb89ULL,
      0x0cef72d2cdef5930ULL, 0xa9e4ca60ebc9a62aULL, 0x8f84dab7d30110cdULL,
      0x6803ad1d2c809a22ULL, 0x4e3319dcbeefdd75ULL, 0xb1c8fe5b88eaedbbULL,
      0x3d5ceaecade87f00ULL },
    { 0xdb87073201334b8eULL, 0x5a37d087e1ae272bULL, 0x80b67cc99b9d27a1ULL,
      0xd09d5f7db214f4baULL, 0x3306a98928d65742ULL, 0x65bc1cc4ac254147ULL,
      0x92e266c8e0c61bd7ULL, 0x3efe1a6dd93a7ea9ULL, 0x516008fcc74b5982ULL,
      0x7087c00b1219938aULL, 0x54ed780d615a4d93ULL, 0xf7b393019e97eb56ULL,
      0x282b3182cb55067cULL, 0xf74719c6e544ea8dULL, 0x35629880486810e2ULL,
      0xccb4db1f1f298264ULL, 0x801d864f04ade5d0ULL, 0xe32ae6267b0c6f71ULL,
      0xc33dbe2c2656b326ULL, 0x8cf005f6aced02b1ULL, 0x1f4f622ce80d5b56ULL,
      0x5c5166351f09065cULL, 0x27ee88da0b3f17f3ULL, 0x24342d232e1cc60dULL,
      0xe71b0ef73358b399ULL, 0x8032078133780469ULL, 0xe34b1780fa310f3dULL,
      0xcde826ad5c866d50ULL, 0x61cbad7ae034ed02ULL, 0x7ec80dc7d5443ff8ULL,
      0x7552b2e6a70d2fefULL, 0xb5c65d387d8a622eULL, 0xcb74a3731b1404dfULL,
      0x3d17820c84c52320ULL, 0x012907839e217ebbULL, 0xeb7f86a56d1ae07aULL,
      0xf934a28184b8bc95ULL, 0xe18c5514a3ffc474ULL, 0x81e302d64f1e874aULL,
      0x993d2d29543a0051ULL, 0x4710980708601e84ULL, 0xc7301ea9cc8b59beULL,
      0x947c354adc6fc86fULL, 0xba7443084a3f3d4dULL, 0x468112e49b318b2eULL,
      0x636e353267f00946ULL, 0xae160a34615f3ce7ULL, 0xd38ce1fbfa0ae670ULL,
      0x183da8dda4081ec1ULL, 0xeccb6d0700512aa9ULL, 0xafffc8e3770d1024ULL,
      0xcc95e0c2786e2d74ULL, 0x7a889c70ef7b2d7bULL, 0xc1612de64cbd613fULL,
      0xf102a29abfbadfefULL, 0x9aa9300a182763faULL, 0x1bc552dcfcfd38a8ULL,
      0x0a7b6521e6b8170dULL, 0x2ba06c62d6b1efcdULL, 0xeea0e6daafd143a8ULL,
      0x9d02cbfd6f7cc234ULL, 0x8aecebc23d68acb5ULL, 0x1d1aa95d783617a7ULL,
      0x514e9d5763edb419ULL },
    { 0x1e8860e4e8afcffcULL, 0x107fb5302e49b653ULL, 0x453ca2eb419de2c5ULL,
      0xa3094e8eb1649123ULL, 0xe850ac9a440dd0afULL, 0xe6eb2d90ade835b6ULL,
      0x75dfa9af6b2e2517ULL, 0x89d272086a4586b0ULL, 0x04f2d543b5fdd518ULL,
      0xec4c28a9a0d6e1e7ULL, 0x17258fcd264c21dbULL, 0xf556bc18377a9614ULL,
      0x1ed98b8d00ed78d9ULL, 0x878cc5541b15235aULL, 0xbdaad05ff19daba8ULL,
      0x92847c097dfa71e5ULL, 0x350e0dd9be966d1fULL, 0xa6a60d7f6f49639cULL,
      0x8e070da95c30c7b1ULL, 0x86cfaab004eeb6baULL, 0xebaaec50d1c327f4ULL,
      0xc2fe1876ab524708ULL, 0xc6f6d37551ec8e3dULL, 0x64d37e4c261b7ceeULL,
      0x5c4238429df044d6ULL, 0xffc2dcfba6020f1cULL, 0x219884780eb85505ULL,
      0x7b344c05cf490f3cULL, 0x6357c42cdd1ece03ULL, 0xe843f0447fc918ccULL,
      0x3f6b9ecca16d6a9cULL, 0xcadd7b5fa7f80548ULL, 0x99932206ba13d48eULL,
      0x2e2c3d8d923d7498ULL, 0x313c5a25c7cc37ecULL, 0x1ad15364afedeef6ULL,
      0xfd191a29a2e31a19ULL, 0x260efed6924e37c5ULL, 0x59131a8c25dde4d2ULL,
      0x7c21c4dec49fcb54ULL, 0x7e6ce786fbf85990ULL, 0xd9e19bec0755de20ULL,
      0xbd6610b15f183afbULL, 0x8973e87e3a7c8151ULL, 0x08dc85a2ef21e267ULL,
      0x8a5d053e38e38217ULL, 0x362b10df55d34595ULL, 0xec947b5836bd99a6ULL,
      0xa86b61318e8e0669ULL, 0x77fb8d13554c503bULL, 0x7f2e977aecd4b847ULL,
      0x1a1209b4297349a0ULL, 0x246ef6a2583a91d2ULL, 0xe899f0f10718bb1bULL,
      0x8caf1fed80b0bf73ULL, 0x6692b681b5b0cd56ULL, 0x88a59e6e5279a693ULL,
      0x2bca6fda2127725bULL, 0xde231627953ee461ULL, 0xa6a84fe6019b1505ULL,
      0x3aa8d5c92821286fULL, 0x3fc7c4ff83788dc8ULL, 0x2f1c86701e11126cULL,
      0xbcf523cd44449d04ULL },
    { 0xb6bd2e675553abfbULL, 0x0e77d5c32d27b9f1ULL, 0x514eeb41d9c3d9ffULL,
      0x628b101131d29234ULL, 0x8ffb7c8ed0e68e19ULL, 0x8ae2aad5fb090e47ULL,
      0x189f977852a4f512ULL, 0x95ca324d8b0efd83ULL, 0x738c57e030fdae11ULL,
      0x959b0a940dd59306ULL, 0x65da2016543c234bULL, 0x4b832a3630d9185cULL,
      0x0c097ea2fdbf0d2cULL, 0xfb4c8e0aa5234704ULL, 0x7ac234e158f27179ULL,
      0xbac5a1e34be49be8ULL, 0x971437a9f55584bbULL, 0x0b936a48e3a2ff38ULL,
      0xce5064cedf5414ffULL, 0xe843d8e0f1a1b404ULL, 0x94b3ca3bb9a34c7fULL,
      0x2ad9eef539a002e0ULL, 0x6775351b325f3972ULL, 0x73a452a5d7816842ULL,
      0x8f820c6a3867e2f5ULL, 0x195da076e4eb0c03ULL, 0xbf06d1e5880ef7e9ULL,
      0x10194dde08d9bfb5ULL, 0xc43bf19aac12b322ULL, 0xe5574447f21ad4fbULL,
      0x0022f70230a30040ULL, 0xb92a66f9542cc415ULL, 0x8fbcd882499ec90dULL,
      0x7d7ee407c482adb5ULL, 0xf284329e4afed0eaULL, 0xa4a200b66d4b9a8fULL,
      0x9d579f70abf43ad5ULL, 0x6d99c3c17a861697ULL, 0xefdfea41ede917ffULL,
      0x0dd85b0a545ce9ebULL, 0x695f389eb81c31c4ULL, 0x3e49b5c0ed676305ULL,
      0x4c0792eaab653521ULL, 0xae7febb40ef265f8ULL, 0x735fd7bae0b300b6ULL,
      0x5cd2f906d01617fdULL, 0xe425e6a2194f35b8ULL, 0x5e454d35558f736fULL,
      0xd6f7a25040a80510ULL, 0x9ff8fd33a560058cULL, 0xc7615283555e4d1aULL,
      0x46403dffc8013c90ULL, 0x4e4a44a1479e3e2eULL, 0xc658ada906c5037bULL,
      0x54e1529f6a6c0706ULL, 0x5a929ed61f0bb3d6ULL, 0x657aacb5a4eef250ULL,
      0x8d75f946b56d5c44ULL, 0x3852aab719722cbaULL, 0xa7e416420eb9da68ULL,
      0x9b7a5005d064dd92ULL, 0xeaf7ce1d22dee993ULL, 0x34aced1247e27fb8ULL,
      0xf23478d46ca33d46ULL }},
   {{ 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL },
    { 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x6b90680243c4b54cULL,
      0x044703d57fb1e299ULL, 0x8a3c60812c08665aULL, 0x32dc7de4cae88a7bULL,
      0x6e3060c4bbed7feeULL, 0x09a012c21457c46fULL, 0x713f791a21ad43caULL,
      0x5a4d0ec873244cdfULL, 0x59a713355c986f8fULL, 0xa347356dba0fbfadULL,
      0x6650c485ba5b1243ULL, 0x2d86094817cc64f2ULL, 0x20fdc446a93f6201ULL,
      0x1db9308f0d27dbdfULL, 0x98f63ae9645b1111ULL, 0x03415b7672cca0bdULL,
      0x910f4a575767cff3ULL, 0x5182927aff4f928cULL, 0xdde2b660565a30dcULL,
      0x3f42ec666558c5b3ULL, 0xce05da712568cac7ULL, 0xfd9326b46e518555ULL,
      0x9334d36563994ecdULL, 0x3bd434362a7b358cULL, 0x14b4d64afc2171bbULL,
      0x8332f03346030a46ULL, 0x56300105aba021c0ULL, 0x470610c2fb63b7b5ULL,
      0x10c12cf0c2f837f5ULL, 0x7d8af8d403969661ULL, 0xa8cb40dbe096915dULL,
      0xfb306c5498354397ULL, 0xe25d9ee093992b8bULL, 0x1f406a7e77f19817ULL,
      0x06592044d4d8c7e7ULL, 0x986ca3c584c84454ULL, 0xcec495f755c884f0ULL,
      0x3d8e0276e94a3fb3ULL, 0x6dfc65f711f0e645ULL, 0xf04572c328e9c0fdULL,
      0x1de908cfe3f5fc3bULL, 0xe1e609e5214fc6f0ULL, 0x1b97e198798ccc46ULL,
      0x52983c479b769f0dULL, 0xe1ad316c24a875e2ULL, 0x759a9b5ac9742a91ULL,
      0x6bb1e5f7b2bb7e47ULL, 0xbdd56e8cc40b30baULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
      0x0000000000000000ULL },
    { 0xe543c7cd3569a2d2ULL, 0x56312cb746ff2365ULL, 0x03e7298e5b86bf22ULL,
      0x516e9a17bf3b9d0eULL, 0xb507b363062bfe41ULL, 0x7600192a061dd0e5ULL,
      0x1d3fdfd7ab7ed9eeULL, 0x422f696efd2a98f8ULL, 0x4d3652c8c76f687cULL,
      0x9f7711eae210b999ULL, 0x3fe55a4a3db93342ULL, 0xf9867077d2c22292ULL,
      0x06938ebd5352aba1ULL, 0xcfbdf8fd28e52d4aULL, 0x9f5dc89b771ea0baULL,
      0x76170ff911d3d955ULL, 0x7e7bddf4e7d92fadULL, 0x28bb97717bb86c3fULL,
      0xe14b10911730bfe5ULL, 0x65fdc8cfbb000784ULL, 0x3413b92f048583c8ULL,
      0x449ccb9483380adcULL, 0xa4a34f7dd630cec5ULL, 0xf23142fcbf290710ULL,
      0xab1645e0b73ebeb6ULL, 0x9f7ac648b66b58efULL, 0x81da786a8e1a3a07ULL,
      0xab80b2cb27644d91ULL, 0xc57b0159cd1cf77aULL, 0x84425693d1e998f6ULL,
      0xa086ad9f1b17e973ULL, 0x6c29c7100c6ea19bULL, 0x3a318a14b98d372bULL,
      0xf01ba582f9382406ULL, 0xa1cde875f370ae9cULL, 0xb9a0ab780158707eULL,
      0x8d26ff790475117eULL, 0xdd33f98a55e26ad9ULL, 0x549c9afccdff9451ULL,
      0xfed35e6c20eabe08ULL, 0xee602ea3d3b08637ULL, 0xfb60f74da1a791f3ULL,
      0xb25a86f4ba75b20eULL, 0x5402f6bd8285b194ULL, 0xb5e9f6533809151fULL,
      0x074c656721f3bb77ULL, 0x5ccdeff9d4bbbf2dULL, 0xbe262f406756394dULL,
      0x105f0a553888caadULL, 0xb364c84f9a7f366dULL, 0x13c5a20c72a2e3ceULL,
      0xb6121d22131c0104ULL, 0xa4cb87927ec6cc17ULL, 0x897b31544122583bULL,
      0xd9bed4853c502a78ULL, 0x211fee372e06645fULL, 0x87d38f49fd841678ULL,
      0x22644edaa636120aULL, 0x693c919475426cccULL, 0x77b3666bd380005dULL,
      0x09972e5b72ba54c1ULL, 0xa6d576eb8a9adbefULL, 0x151e128a1470cc43ULL,
      0xba872ea0ccbcac7cULL },
    { 0xfaadf26b0bedcb90ULL, 0x577ac5a51e5651c5ULL, 0xfaa9928d90ecc574ULL,
      0xf4d0746e2cd9c2cbULL, 0x297b213cebe4451bULL, 0xdcda00e8a1970957ULL,
      0xebc24328279cf3c0ULL, 0xafbc7b8a2782f71aULL, 0x2b8ae9d228a44b14ULL,
      0x7b91361891e74156ULL, 0x7c744bd7713ebc3cULL, 0xf30ff94d8ae2dbbcULL,
      0x952d28d81c4cd5e5ULL, 0xca1da973d1bb1a14ULL, 0x0dc59391a4b26780ULL,
      0x73974085e2ebdfedULL, 0x6a9d797790afccadULL, 0x192a2b602b123aabULL,
      0x19c8a787015bc845ULL, 0xad40920248a2e551ULL, 0xd1c0fbdcb077da5dULL,
      0xf0d3a2e4b44b9030ULL, 0x4ff44c34da7b4dd0ULL, 0x65ca4e0e02964227ULL,
      0x1fc804c725e40440ULL, 0xde8e75b6789090ffULL, 0xda82400db9d07a0aULL,
      0x7f850c159d730e8dULL, 0x88f3cb3c8b5dbf18ULL, 0xaa16f0d4828d0050ULL,
      0x46a26a52fc055ad2ULL, 0x9924d71d3d7359bfULL, 0x73580533d9f2fb99ULL,
      0x187096a491259118ULL, 0x7777434ec63d90f8ULL, 0x8e6e9922b252bdb1ULL,
      0xb1af461d516eee12ULL, 0x4fcd31f2f421c717ULL, 0x65651bc93644c5e1ULL,
      0x9bd1e1fa908307daULL, 0x5aaee550208ae7f0ULL, 0x4ec958544caaa9c3ULL,
      0x298e4a3f09caee63ULL, 0x29d4a3cab10a9a44ULL, 0x9e6a3fde44907510ULL,
      0x19cc5ee9aa4fbc78ULL, 0xc01a289fe006387fULL, 0x3ee6737f5f934fb3ULL,
      0x65fad64d232fe9eeULL, 0x2a487fc2e4f569fbULL, 0x67fb5df086391215ULL,
      0xc1b5e63af64d55e8ULL, 0xd33f764f4052ecb7ULL, 0x0899d25e2b391f79ULL,
      0xc70158cce44d2e70ULL, 0xb53262e2308fa659ULL, 0x20f7402ecd84404cULL,
      0x15d1b9bb5466ad4cULL, 0xfd8d825e26a8a2d1ULL, 0x18d96f18f8b826d0ULL,
      0xf72067659687ef21ULL, 0xf08610bafd66009fULL, 0x1a4e1ba1fdb05563ULL,
      0x1324d44c84bbef0aULL },
    { 0xc7613d66f20232d4ULL, 0x3ede983f7b06516bULL, 0x578460a649b24a39ULL,
      0xd5bd4ad2cc618853ULL, 0x6da1e9f12833259cULL, 0x1f9ca81ea33005a0ULL,
      0xae981ca25036ed10ULL, 0x5d69e428228cca9aULL, 0xa17023dd34f22effULL,
      0xd08c493917325f88ULL, 0x79e16b54bdca7e02ULL, 0x3654be3ac3a26289ULL,
      0xac56a76157fd0921ULL, 0xc517aa54a54ed12dULL, 0x4dd1b68eb1ada829ULL,
      0x161037898e176e8dULL, 0x98db096afb729bebULL, 0x03e9d68eabc9d7feULL,
      0xdf27c2fb173845dcULL, 0x2aee2474d7bf2d7cULL, 0xf5d6d12955e96f2dULL,
      0xf734ec0a2d041943ULL, 0x72999224bb580060ULL, 0xccdcad5bdc7bd1a1ULL,
      0xa9bca8004f1d7086ULL, 0xc2c7ffb795d4e1e7ULL, 0xd28da598cbfbc3caULL,
      0x08126a5da13c42d9ULL, 0x13e15ad5c2d3d951ULL, 0x4d9c20c7eaa8703dULL,
      0xab894b4e6ebca682ULL, 0x53214a10ad7c784eULL, 0x9906d6c9c38b591eULL,
      0x476bd91a4ca0c161ULL, 0x95265cbe69f01d02ULL, 0x01cedc5a39e5b5c9ULL,
      0xc0ef0788f08d9463ULL, 0x80927db966612b20ULL, 0x9630082145dd2f1eULL,
      0x600f737ed7910113ULL, 0xb9302c026f2a80b2ULL, 0xff49c8f0afc01a91ULL,
      0x39a0b9dbbc8e5d10ULL, 0x4e93e5fbc38cafd7ULL, 0x44d0aec75c666288ULL,
      0x5ea41ef6508a97efULL, 0x6ce58a3a4917dffdULL, 0x57b84b8764d3da3dULL,
      0xedf320cbe664658cULL, 0x3bea7e195d96172aULL, 0x72abd9c2b876c142ULL,
      0xaf2ce404a46dae6eULL, 0x4f050de918e728b2ULL, 0xcb458cba72881a48ULL,
      0xdadcfcb48f1c02a8ULL, 0x1167bf4ccb9f1a34ULL, 0x2f791f047047bd5dULL,
      0xcd60c5789e26b8b6ULL, 0x1a620288f4a12b4bULL, 0x1b43a4a5b7f5244dULL,
      0x3a93e22c7f6b3c55ULL, 0xa5da9b1ba501f044ULL, 0xac918f8b4d9d1e00ULL,
      0x9fba7867a63c8ac8ULL },
    { 0xb5663909d9d6303aULL, 0x1122bbd8f31a9801ULL, 0xb26dfdc7254c0ac9ULL,
      0x80923ffe2bae8db2ULL, 0xdf3939952977e95fULL, 0x89ef22889d7081a6ULL,
      0xddeeb25f2e41e526ULL, 0x77aa072d06890a48ULL, 0x8b1e7a1bc43beb1cULL,
      0xeef27b1803a60f3aULL, 0x116569c8ba82a83aULL, 0x861eddf3fdf4d64fULL,
      0x12fc5033c7c95105ULL, 0xc9997d1a2f05161aULL, 0x56dbac3597f7e48bULL,
      0x997968b0dadb71b6ULL, 0x51c153dd787cf748ULL, 0x190b253068b60e60ULL,
      0xa98dcc93091ee1daULL, 0xe7f1c48a17f0c994ULL, 0x294532d1f3b50a20ULL,
      0xe393a663d106d4b2ULL, 0xc95a8e15d4e4aac2ULL, 0x058a1a4819387af4ULL,
      0xb7fe4077025d3331ULL, 0x17369b1ad4dfb135ULL, 0xfd1836fd44897416ULL,
      0x734f98ea7a95ea1dULL, 0x44dbebde2de33051ULL, 0x3e572ff979d8ca38ULL,
      0xd53c5a45bb8cdfa1ULL, 0x1169ade998830992ULL, 0xc6b5b477d2ee43b5ULL,
      0x11d58b895af5f73aULL, 0xcfe3985db2d35d21ULL, 0xc9c6056490e28221ULL,
      0xbba44fb18d7bad4cULL, 0xd1b94354c3d22c4dULL, 0xfb0d0d4c55eedddeULL,
      0x3b18e9ef00b4c810ULL, 0x73e101f840df8084ULL, 0xd64f148443724752ULL,
      0xb017cbfb12688bd6ULL, 0x89e6a53131fc7242ULL, 0x2f6bc2d724af9792ULL,
      0x1af46d6374011c6aULL, 0xe7d461f15c6129b4ULL, 0xbd7b0f8478d446abULL,
      0x8cab2463b6c0e01dULL, 0xa69dee16a765d2b0ULL, 0x144588401f496bf3ULL,
      0x3d761d20063ee258ULL, 0x48c0b32df8ddc0fbULL, 0x1ad8889a5aa8e26cULL,
      0x2aadb6180f80c2d3ULL, 0xb7c9d582a54c0b2cULL, 0xa9448d0f698b8370ULL,
      0xd6814d04b2584c63ULL, 0x80576b83319d83f9ULL, 0x906953398a3df494ULL,
      0xe7a11b9d7d769494ULL, 0x59714b37b93b5e39ULL, 0xa5280a61ef2d0450ULL,
      0xcaf1ca6cd004e7bcULL },
    { 0x0c92df7ab48210acULL, 0x70c766782c6225c0ULL, 0x2b627e280a8dd01fULL,
      0xf8a95606e064f51bULL, 0xecb6d461dd6c8568ULL, 0xe8e9d8da88a760e7ULL,
      0xb253ddc5e1b54ff4ULL, 0xb518eacb142499c5ULL, 0x5ba23807ace2576bULL,
      0xb5d274ebd42fcb9cULL, 0x1dfc510cd7016641ULL, 0x81b2aa898d7ff740ULL,
      0xb3f8b22a412d350fULL, 0x9010d26bd30013d6ULL, 0x31a160801ed3585fULL,
      0xf18717011ddb123bULL, 0x475fb6cd262d3895ULL, 0xf86a9bada37fe981ULL,
      0x579fe8f10c63060cULL, 0x1ea46e3bcbee6f47ULL, 0x0dfa846a5626e47aULL,
      0xe76ff8e4aab118a0ULL, 0xa83a45a05758d1c4ULL, 0xff293f1d1de94a79ULL,
      0x6d34106328ce50acULL, 0x7f3dd6bb2c715f0dULL, 0x01a6483cc3fb62c0ULL,
      0xa60927ade8ccdca7ULL, 0x73c1dc8d32c0180fULL, 0x02f86bcc14474ff9ULL,
      0x3804de0c37c58434ULL, 0xfc10f3a3c497c54dULL, 0x96a1e55142ddb8dbULL,
      0xc92548d8939af17aULL, 0xbc5edf6509acaf89ULL, 0xac0b9688d3023544ULL,
      0x4163dbff847088b3ULL, 0x563f3cfce243d3f8ULL, 0x8f263f7ce1f7b3ccULL,
      0xe7365cbc6a7f8730ULL, 0x46c1f063e6b8ca39ULL, 0x21cbc42ba4582264ULL,
      0x55dff0476966e4b4ULL, 0x223e2c38b61edc4eULL, 0x3a21ced28a3d6fa8ULL,
      0xd5884ee48c058d27ULL, 0x884d4eac614c987aULL, 0x02327b02f1a6a37fULL,
      0xec14f49f926b0d1dULL, 0xad980ec0c9b3ccdcULL, 0xad0f89f58a31f96cULL,
      0x1004ad6869a8c64dULL, 0x73334f7053ecda9cULL, 0xe5c726eb2395f91fULL,
      0x3eacccee6b0d2cf0ULL, 0x54fe44475c2803fbULL, 0x4e691ecbfdff4c35ULL,
      0x0d4dd3188efdf8f3ULL, 0x4ce513f999517686ULL, 0x451033ddedb79722ULL,
      0x7fafe619290cf26eULL, 0xb744be4992d915b4ULL, 0x8011bf394690d8d1ULL,
      0x9475361b15b45cd5ULL },
  },
};
BITBOARD castle_random[2][2] = {
  { 0x557723689550b69bULL, 0xa92c541efa336c6cULL },
  { 0xc7bedab779d5361bULL, 0x3bb70e80435e60b7ULL }

};
BITBOARD enpassant_random[65] = {
  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
  0x0000000000000000ULL, 0xf2c1412f44c13c98ULL, 0x76b4b7ccb0c9bd1eULL,
  0x0303f047ef3166cdULL, 0xcf4da3850ff5c35aULL, 0x0bb57340632ec140ULL,
  0x189156c368616498ULL, 0x71b862b8cede277dULL, 0x26e0433817e6d7d7ULL,
  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
  0x0000000000000000ULL, 0x1f2af0448165ab3aULL, 0x51f0d423276d44dbULL,
  0x12d51a6ba742f661ULL, 0x8fa3e91c53630e1fULL, 0x16573a4eb7f48c08ULL,
  0xe1c1e4bc9690e409ULL, 0x5f2bf4422dde33bbULL, 0xcd4cefba64f407a1ULL,
  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
  0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
  0x0000000000000000ULL, 0x0000000000000000ULL
};
BITBOARD pawn_attacks[2][64];
BITBOARD knight_attacks[64];
BITBOARD rook_attacks[64];
BITBOARD bishop_attacks[64];
BITBOARD king_attacks[64];
BITBOARD intervening[64][64];
BITBOARD clear_mask[65];
BITBOARD set_mask[65];
BITBOARD file_mask[8];
BITBOARD rank_mask[8];
BITBOARD OO[2] = { 0x6000000000000000ULL, 0x0000000000000060ULL };
BITBOARD OOO[2] = { 0x0E00000000000000ULL, 0x000000000000000EULL };
BITBOARD mask_efgh, mask_fgh, mask_abc, mask_abcd;
BITBOARD mask_advance_2_w;
BITBOARD mask_advance_2_b;
BITBOARD mask_left_edge;
BITBOARD mask_right_edge;
BITBOARD mask_not_edge;
BITBOARD mask_kr_trapped[2][3];
BITBOARD mask_qr_trapped[2][3];
BITBOARD dark_squares;
BITBOARD not_rook_pawns;
BITBOARD rook_pawns;
BITBOARD plus1dir[65];
BITBOARD plus7dir[65];
BITBOARD plus8dir[65];
BITBOARD plus9dir[65];
BITBOARD minus1dir[65];
BITBOARD minus7dir[65];
BITBOARD minus8dir[65];
BITBOARD minus9dir[65];
BITBOARD mask_eptest[64];
BITBOARD mask_clear_entry;
POSITION display;
#if (!defined(_M_AMD64) && !defined (_M_IA64) && !defined(INLINE32)) || defined(VC_INLINE32)
unsigned char msb[65536];
unsigned char lsb[65536];
#endif
unsigned char msb_8bit[256];
unsigned char lsb_8bit[256];
unsigned char pop_cnt_8bit[256];
signed char is_outside[256][256];
BITBOARD mask_pawn_connected[64];
BITBOARD mask_pawn_duo[64];
BITBOARD mask_pawn_isolated[64];
BITBOARD mask_passed[2][64];
BITBOARD mask_no_pattacks[2][64];
BITBOARD mask_hidden_left[2][8];
BITBOARD mask_hidden_right[2][8];
BITBOARD pawn_race[2][2][64];
BOOK_POSITION book_buffer[BOOK_CLUSTER_SIZE];
BOOK_POSITION book_buffer_char[BOOK_CLUSTER_SIZE];
int OOsqs[2][3] = {{ E8, F8, G8 }, { E1, F1, G1 }};
int OOOsqs[2][3] = {{ E8, D8, C8 }, { E1, D1, C1 }};
int OOfrom[2] = { E8, E1 };
int OOto[2] = { G8, G1 };
int OOOto[2] = { C8, C1 };
#define    VERSION                             "23.3"
char version[8] = { VERSION };
PLAYING_MODE mode = normal_mode;
int batch_mode = 0;             /* no asynch reads */
int swindle_mode = 1;           /* try to swindle */
int call_flag = 0;
int crafty_rating = 2500;
int opponent_rating = 2500;
int last_search_value = 0;
int lazy_eval_cutoff = 125;
int razor_margin = 300;
int pruning_margin[8] = {0, 125, 125, 300, 300, 400, 400, 500};
int pruning_depth = 5;
int pgn_suggested_percent = 0;
char pgn_event[128] = { "?" };
char pgn_site[128] = { "?" };
char pgn_date[128] = { "????.??.??" };
char pgn_round[128] = { "?" };
char pgn_white[128] = { "unknown" };
char pgn_white_elo[128] = { "" };
char pgn_black[128] = { "Crafty " VERSION };
char pgn_black_elo[128] = { "" };
char pgn_result[128] = { "*" };
char *B_list[128];
char *AK_list[128];
char *C_list[128];
char *GM_list[128];
char *IM_list[128];
char *SP_list[128];
char *SP_opening_filename[128];
char *SP_personality_filename[128];
int output_format = 0;
#if !defined(NOEGTB)
int EGTBlimit = 0;
int EGTB_use = 0;
int EGTB_draw = 0;
int EGTB_search = 0;
size_t EGTB_cache_size = EGTB_CACHE_DEFAULT;
void *EGTB_cache = (void *) 0;
int EGTB_setup = 0;
#endif
int xboard = 0;
int pong = 0;
int channel = 0;
int early_exit = 99;
int new_game = 0;
char channel_title[32] = { "" };
char book_path[128] = { BOOKDIR };
char log_path[128] = { LOGDIR };
char tb_path[128] = { TBDIR };
char rc_path[128] = { RCDIR };
int initialized = 0;
int kibitz = 0;
int post = 0;
int log_id = 0;
int wtm = 1;
int last_opponent_move = 0;
int check_depth = 1;
int null_depth = 3;               /* R=3 */
int LMR_remaining_depth = 1;      /* leave 1 full ply after reductions */
int LMR_min_reduction = 1;        /* minimum reduction 1 ply */
int LMR_max_reduction = 2;        /* maximum reduction 2 plies */
int search_depth = 0;
unsigned int search_nodes = 0;
unsigned int temp_search_nodes = 0;
int search_move = 0;
int predicted = 0;
int time_used = 0;
int time_used_opponent = 0;
int analyze_mode = 0;
int annotate_mode = 0;
int input_status = 0;
int resign = 9;
int resign_counter = 0;
int resign_count = 5;
int draw_counter = 0;
int draw_count = 5;
int draw_offer_pending = 0;
int draw_offered = 0;
int offer_draws = 1;
int adaptive_hash = 0;
size_t adaptive_hash_min = 0;
size_t adaptive_hash_max = 0;
size_t adaptive_hashp_min = 0;
size_t adaptive_hashp_max = 0;
int time_limit = 100;
int force = 0;
char initial_position[80] = { "" };
char hint[512] = { "" };
char book_hint[512] = { "" };
int over = 0;
int silent = 0;
int usage_level = 0;
char audible_alarm = 0x07;
char speech = 0;
int book_accept_mask = ~03;
int book_reject_mask = 3;
int book_random = 1;
float book_weight_learn = 1.0;
float book_weight_freq = 1.0;
float book_weight_eval = 0.1;
int book_search_trigger = 20;
int learning = 1;
int learn_value = 0;
int time_abort;
int abort_search;
int iteration_depth;
int root_alpha;
int root_beta;
int root_value;
int root_wtm;
int last_root_value;
ROOT_MOVE root_moves[256];
int n_root_moves;
int easy_move;
int absolute_time_limit;
int search_time_limit;
int burp;
int quit = 0;
unsigned int opponent_start_time, opponent_end_time;
unsigned int program_start_time, program_end_time;
unsigned int start_time, end_time;
unsigned int elapsed_start, elapsed_end;
TREE *block[MAX_BLOCKS + 1];
TREE *volatile thread[CPUS];
#if (CPUS > 1)
lock_t lock_smp, lock_io, lock_root;
#if defined(UNIX)
  pthread_attr_t attributes;
#endif
#endif
unsigned int parallel_splits;
unsigned int parallel_aborts;
unsigned int max_split_blocks;
volatile int smp_idle = 0;
volatile int smp_threads = 0;
volatile int initialized_threads = 0;
int crafty_is_white = 0;
int nodes_between_time_checks = 1000000;
int nodes_per_second = 1000000;
int next_time_check = 100000;
int transposition_id = 0;
int thinking = 0;
int pondering = 0;
int puzzling = 0;
int booking = 0;
int trojan_check = 0;
int computer_opponent = 0;
int display_options = 4095 - 256 - 512;
int smp_max_threads = 0;
int smp_max_thread_group = 4;
int smp_split_at_root = 1;
int smp_split_nodes = 1000;
unsigned int noise_level = 200000;
int tc_moves = 60;
int tc_time = 180000;
int tc_time_remaining[2] = { 180000, 180000 };
int tc_moves_remaining[2] = { 60, 60 };
int tc_secondary_moves = 30;
int tc_secondary_time = 90000;
int tc_increment = 0;
int tc_sudden_death = 0;
int tc_operator_time = 0;
int tc_safety_margin = 0;
int draw_score[2] = { 0, 0 };
char kibitz_text[512];
int kibitz_depth;
int move_number = 1;
int root_print_ok = 0;
int moves_out_of_book = 0;
int first_nonbook_factor = 0;
int first_nonbook_span = 0;
int smp_nice = 1;
#if defined(SKILL)
int skill = 100;
#endif
int show_book = 0;
int book_selection_width = 5;
int ponder = 1;
int trace_level = 0;
/*  hash_mask should be (hash_table_size - 1) >> 2  */
size_t hash_table_size = 65536;
BITBOARD hash_mask = 0x00003fff;
/*  pawn_hash_mask should be pawn_hash_table_size - 1  */
size_t pawn_hash_table_size = 16384;
BITBOARD pawn_hash_mask = 0x00003fff;
int abs_draw_score = 1;
int accept_draws = 1;
const char translate[13] =
    { 'k', 'q', 'r', 'b', 'n', 'p', 0, 'P', 'N', 'B', 'R', 'Q', 'K' };
BITBOARD magic_bishop[64] = {
  0x0002020202020200ULL, 0x0002020202020000ULL, 0x0004010202000000ULL,
  0x0004040080000000ULL, 0x0001104000000000ULL, 0x0000821040000000ULL,
  0x0000410410400000ULL, 0x0000104104104000ULL, 0x0000040404040400ULL,
  0x0000020202020200ULL, 0x0000040102020000ULL, 0x0000040400800000ULL,
  0x0000011040000000ULL, 0x0000008210400000ULL, 0x0000004104104000ULL,
  0x0000002082082000ULL, 0x0004000808080800ULL, 0x0002000404040400ULL,
  0x0001000202020200ULL, 0x0000800802004000ULL, 0x0000800400A00000ULL,
  0x0000200100884000ULL, 0x0000400082082000ULL, 0x0000200041041000ULL,
  0x0002080010101000ULL, 0x0001040008080800ULL, 0x0000208004010400ULL,
  0x0000404004010200ULL, 0x0000840000802000ULL, 0x0000404002011000ULL,
  0x0000808001041000ULL, 0x0000404000820800ULL, 0x0001041000202000ULL,
  0x0000820800101000ULL, 0x0000104400080800ULL, 0x0000020080080080ULL,
  0x0000404040040100ULL, 0x0000808100020100ULL, 0x0001010100020800ULL,
  0x0000808080010400ULL, 0x0000820820004000ULL, 0x0000410410002000ULL,
  0x0000082088001000ULL, 0x0000002011000800ULL, 0x0000080100400400ULL,
  0x0001010101000200ULL, 0x0002020202000400ULL, 0x0001010101000200ULL,
  0x0000410410400000ULL, 0x0000208208200000ULL, 0x0000002084100000ULL,
  0x0000000020880000ULL, 0x0000001002020000ULL, 0x0000040408020000ULL,
  0x0004040404040000ULL, 0x0002020202020000ULL, 0x0000104104104000ULL,
  0x0000002082082000ULL, 0x0000000020841000ULL, 0x0000000000208800ULL,
  0x0000000010020200ULL, 0x0000000404080200ULL, 0x0000040404040400ULL,
  0x0002020202020200ULL
};
BITBOARD magic_bishop_mask[64] = {
  0x0040201008040200ULL, 0x0000402010080400ULL, 0x0000004020100A00ULL,
  0x0000000040221400ULL, 0x0000000002442800ULL, 0x0000000204085000ULL,
  0x0000020408102000ULL, 0x0002040810204000ULL, 0x0020100804020000ULL,
  0x0040201008040000ULL, 0x00004020100A0000ULL, 0x0000004022140000ULL,
  0x0000000244280000ULL, 0x0000020408500000ULL, 0x0002040810200000ULL,
  0x0004081020400000ULL, 0x0010080402000200ULL, 0x0020100804000400ULL,
  0x004020100A000A00ULL, 0x0000402214001400ULL, 0x0000024428002800ULL,
  0x0002040850005000ULL, 0x0004081020002000ULL, 0x0008102040004000ULL,
  0x0008040200020400ULL, 0x0010080400040800ULL, 0x0020100A000A1000ULL,
  0x0040221400142200ULL, 0x0002442800284400ULL, 0x0004085000500800ULL,
  0x0008102000201000ULL, 0x0010204000402000ULL, 0x0004020002040800ULL,
  0x0008040004081000ULL, 0x00100A000A102000ULL, 0x0022140014224000ULL,
  0x0044280028440200ULL, 0x0008500050080400ULL, 0x0010200020100800ULL,
  0x0020400040201000ULL, 0x0002000204081000ULL, 0x0004000408102000ULL,
  0x000A000A10204000ULL, 0x0014001422400000ULL, 0x0028002844020000ULL,
  0x0050005008040200ULL, 0x0020002010080400ULL, 0x0040004020100800ULL,
  0x0000020408102000ULL, 0x0000040810204000ULL, 0x00000A1020400000ULL,
  0x0000142240000000ULL, 0x0000284402000000ULL, 0x0000500804020000ULL,
  0x0000201008040200ULL, 0x0000402010080400ULL, 0x0002040810204000ULL,
  0x0004081020400000ULL, 0x000A102040000000ULL, 0x0014224000000000ULL,
  0x0028440200000000ULL, 0x0050080402000000ULL, 0x0020100804020000ULL,
  0x0040201008040200ULL
};
unsigned int magic_bishop_shift[64] = {
  58, 59, 59, 59, 59, 59, 59, 58,
  59, 59, 59, 59, 59, 59, 59, 59,
  59, 59, 57, 57, 57, 57, 59, 59,
  59, 59, 57, 55, 55, 57, 59, 59,
  59, 59, 57, 55, 55, 57, 59, 59,
  59, 59, 57, 57, 57, 57, 59, 59,
  59, 59, 59, 59, 59, 59, 59, 59,
  58, 59, 59, 59, 59, 59, 59, 58
};
BITBOARD magic_bishop_table[5248];
BITBOARD *magic_bishop_indices[64] = {
  magic_bishop_table + 4992, magic_bishop_table + 2624,
  magic_bishop_table + 256, magic_bishop_table + 896,
  magic_bishop_table + 1280, magic_bishop_table + 1664,
  magic_bishop_table + 4800, magic_bishop_table + 5120,
  magic_bishop_table + 2560, magic_bishop_table + 2656,
  magic_bishop_table + 288, magic_bishop_table + 928,
  magic_bishop_table + 1312, magic_bishop_table + 1696,
  magic_bishop_table + 4832, magic_bishop_table + 4928,
  magic_bishop_table + 0, magic_bishop_table + 128,
  magic_bishop_table + 320, magic_bishop_table + 960,
  magic_bishop_table + 1344, magic_bishop_table + 1728,
  magic_bishop_table + 2304, magic_bishop_table + 2432,
  magic_bishop_table + 32, magic_bishop_table + 160,
  magic_bishop_table + 448, magic_bishop_table + 2752,
  magic_bishop_table + 3776, magic_bishop_table + 1856,
  magic_bishop_table + 2336, magic_bishop_table + 2464,
  magic_bishop_table + 64, magic_bishop_table + 192,
  magic_bishop_table + 576, magic_bishop_table + 3264,
  magic_bishop_table + 4288, magic_bishop_table + 1984,
  magic_bishop_table + 2368, magic_bishop_table + 2496,
  magic_bishop_table + 96, magic_bishop_table + 224,
  magic_bishop_table + 704, magic_bishop_table + 1088,
  magic_bishop_table + 1472, magic_bishop_table + 2112,
  magic_bishop_table + 2400, magic_bishop_table + 2528,
  magic_bishop_table + 2592, magic_bishop_table + 2688,
  magic_bishop_table + 832, magic_bishop_table + 1216,
  magic_bishop_table + 1600, magic_bishop_table + 2240,
  magic_bishop_table + 4864, magic_bishop_table + 4960,
  magic_bishop_table + 5056, magic_bishop_table + 2720,
  magic_bishop_table + 864, magic_bishop_table + 1248,
  magic_bishop_table + 1632, magic_bishop_table + 2272,
  magic_bishop_table + 4896, magic_bishop_table + 5184
};
short int magic_bishop_mobility_table[5248];
short int *magic_bishop_mobility_indices[64] = {
  magic_bishop_mobility_table + 4992, magic_bishop_mobility_table + 2624,
  magic_bishop_mobility_table + 256, magic_bishop_mobility_table + 896,
  magic_bishop_mobility_table + 1280, magic_bishop_mobility_table + 1664,
  magic_bishop_mobility_table + 4800, magic_bishop_mobility_table + 5120,
  magic_bishop_mobility_table + 2560, magic_bishop_mobility_table + 2656,
  magic_bishop_mobility_table + 288, magic_bishop_mobility_table + 928,
  magic_bishop_mobility_table + 1312, magic_bishop_mobility_table + 1696,
  magic_bishop_mobility_table + 4832, magic_bishop_mobility_table + 4928,
  magic_bishop_mobility_table + 0, magic_bishop_mobility_table + 128,
  magic_bishop_mobility_table + 320, magic_bishop_mobility_table + 960,
  magic_bishop_mobility_table + 1344, magic_bishop_mobility_table + 1728,
  magic_bishop_mobility_table + 2304, magic_bishop_mobility_table + 2432,
  magic_bishop_mobility_table + 32, magic_bishop_mobility_table + 160,
  magic_bishop_mobility_table + 448, magic_bishop_mobility_table + 2752,
  magic_bishop_mobility_table + 3776, magic_bishop_mobility_table + 1856,
  magic_bishop_mobility_table + 2336, magic_bishop_mobility_table + 2464,
  magic_bishop_mobility_table + 64, magic_bishop_mobility_table + 192,
  magic_bishop_mobility_table + 576, magic_bishop_mobility_table + 3264,
  magic_bishop_mobility_table + 4288, magic_bishop_mobility_table + 1984,
  magic_bishop_mobility_table + 2368, magic_bishop_mobility_table + 2496,
  magic_bishop_mobility_table + 96, magic_bishop_mobility_table + 224,
  magic_bishop_mobility_table + 704, magic_bishop_mobility_table + 1088,
  magic_bishop_mobility_table + 1472, magic_bishop_mobility_table + 2112,
  magic_bishop_mobility_table + 2400, magic_bishop_mobility_table + 2528,
  magic_bishop_mobility_table + 2592, magic_bishop_mobility_table + 2688,
  magic_bishop_mobility_table + 832, magic_bishop_mobility_table + 1216,
  magic_bishop_mobility_table + 1600, magic_bishop_mobility_table + 2240,
  magic_bishop_mobility_table + 4864, magic_bishop_mobility_table + 4960,
  magic_bishop_mobility_table + 5056, magic_bishop_mobility_table + 2720,
  magic_bishop_mobility_table + 864, magic_bishop_mobility_table + 1248,
  magic_bishop_mobility_table + 1632, magic_bishop_mobility_table + 2272,
  magic_bishop_mobility_table + 4896, magic_bishop_mobility_table + 5184
};
BITBOARD magic_rook_table[102400];
BITBOARD *magic_rook_indices[64] = {
  magic_rook_table + 86016, magic_rook_table + 73728,
  magic_rook_table + 36864, magic_rook_table + 43008,
  magic_rook_table + 47104, magic_rook_table + 51200,
  magic_rook_table + 77824, magic_rook_table + 94208,
  magic_rook_table + 69632, magic_rook_table + 32768,
  magic_rook_table + 38912, magic_rook_table + 10240,
  magic_rook_table + 14336, magic_rook_table + 53248,
  magic_rook_table + 57344, magic_rook_table + 81920,
  magic_rook_table + 24576, magic_rook_table + 33792,
  magic_rook_table + 6144, magic_rook_table + 11264,
  magic_rook_table + 15360, magic_rook_table + 18432,
  magic_rook_table + 58368, magic_rook_table + 61440,
  magic_rook_table + 26624, magic_rook_table + 4096,
  magic_rook_table + 7168, magic_rook_table + 0,
  magic_rook_table + 2048, magic_rook_table + 19456,
  magic_rook_table + 22528, magic_rook_table + 63488,
  magic_rook_table + 28672, magic_rook_table + 5120,
  magic_rook_table + 8192, magic_rook_table + 1024,
  magic_rook_table + 3072, magic_rook_table + 20480,
  magic_rook_table + 23552, magic_rook_table + 65536,
  magic_rook_table + 30720, magic_rook_table + 34816,
  magic_rook_table + 9216, magic_rook_table + 12288,
  magic_rook_table + 16384, magic_rook_table + 21504,
  magic_rook_table + 59392, magic_rook_table + 67584,
  magic_rook_table + 71680, magic_rook_table + 35840,
  magic_rook_table + 39936, magic_rook_table + 13312,
  magic_rook_table + 17408, magic_rook_table + 54272,
  magic_rook_table + 60416, magic_rook_table + 83968,
  magic_rook_table + 90112, magic_rook_table + 75776,
  magic_rook_table + 40960, magic_rook_table + 45056,
  magic_rook_table + 49152, magic_rook_table + 55296,
  magic_rook_table + 79872, magic_rook_table + 98304
};
short int magic_rook_mobility_table[102400];
short int *magic_rook_mobility_indices[64] = {
  magic_rook_mobility_table + 86016, magic_rook_mobility_table + 73728,
  magic_rook_mobility_table + 36864, magic_rook_mobility_table + 43008,
  magic_rook_mobility_table + 47104, magic_rook_mobility_table + 51200,
  magic_rook_mobility_table + 77824, magic_rook_mobility_table + 94208,
  magic_rook_mobility_table + 69632, magic_rook_mobility_table + 32768,
  magic_rook_mobility_table + 38912, magic_rook_mobility_table + 10240,
  magic_rook_mobility_table + 14336, magic_rook_mobility_table + 53248,
  magic_rook_mobility_table + 57344, magic_rook_mobility_table + 81920,
  magic_rook_mobility_table + 24576, magic_rook_mobility_table + 33792,
  magic_rook_mobility_table + 6144, magic_rook_mobility_table + 11264,
  magic_rook_mobility_table + 15360, magic_rook_mobility_table + 18432,
  magic_rook_mobility_table + 58368, magic_rook_mobility_table + 61440,
  magic_rook_mobility_table + 26624, magic_rook_mobility_table + 4096,
  magic_rook_mobility_table + 7168, magic_rook_mobility_table + 0,
  magic_rook_mobility_table + 2048, magic_rook_mobility_table + 19456,
  magic_rook_mobility_table + 22528, magic_rook_mobility_table + 63488,
  magic_rook_mobility_table + 28672, magic_rook_mobility_table + 5120,
  magic_rook_mobility_table + 8192, magic_rook_mobility_table + 1024,
  magic_rook_mobility_table + 3072, magic_rook_mobility_table + 20480,
  magic_rook_mobility_table + 23552, magic_rook_mobility_table + 65536,
  magic_rook_mobility_table + 30720, magic_rook_mobility_table + 34816,
  magic_rook_mobility_table + 9216, magic_rook_mobility_table + 12288,
  magic_rook_mobility_table + 16384, magic_rook_mobility_table + 21504,
  magic_rook_mobility_table + 59392, magic_rook_mobility_table + 67584,
  magic_rook_mobility_table + 71680, magic_rook_mobility_table + 35840,
  magic_rook_mobility_table + 39936, magic_rook_mobility_table + 13312,
  magic_rook_mobility_table + 17408, magic_rook_mobility_table + 54272,
  magic_rook_mobility_table + 60416, magic_rook_mobility_table + 83968,
  magic_rook_mobility_table + 90112, magic_rook_mobility_table + 75776,
  magic_rook_mobility_table + 40960, magic_rook_mobility_table + 45056,
  magic_rook_mobility_table + 49152, magic_rook_mobility_table + 55296,
  magic_rook_mobility_table + 79872, magic_rook_mobility_table + 98304
};
BITBOARD magic_rook[64] = {
  0x0080001020400080ULL, 0x0040001000200040ULL, 0x0080081000200080ULL,
  0x0080040800100080ULL, 0x0080020400080080ULL, 0x0080010200040080ULL,
  0x0080008001000200ULL, 0x0080002040800100ULL, 0x0000800020400080ULL,
  0x0000400020005000ULL, 0x0000801000200080ULL, 0x0000800800100080ULL,
  0x0000800400080080ULL, 0x0000800200040080ULL, 0x0000800100020080ULL,
  0x0000800040800100ULL, 0x0000208000400080ULL, 0x0000404000201000ULL,
  0x0000808010002000ULL, 0x0000808008001000ULL, 0x0000808004000800ULL,
  0x0000808002000400ULL, 0x0000010100020004ULL, 0x0000020000408104ULL,
  0x0000208080004000ULL, 0x0000200040005000ULL, 0x0000100080200080ULL,
  0x0000080080100080ULL, 0x0000040080080080ULL, 0x0000020080040080ULL,
  0x0000010080800200ULL, 0x0000800080004100ULL, 0x0000204000800080ULL,
  0x0000200040401000ULL, 0x0000100080802000ULL, 0x0000080080801000ULL,
  0x0000040080800800ULL, 0x0000020080800400ULL, 0x0000020001010004ULL,
  0x0000800040800100ULL, 0x0000204000808000ULL, 0x0000200040008080ULL,
  0x0000100020008080ULL, 0x0000080010008080ULL, 0x0000040008008080ULL,
  0x0000020004008080ULL, 0x0000010002008080ULL, 0x0000004081020004ULL,
  0x0000204000800080ULL, 0x0000200040008080ULL, 0x0000100020008080ULL,
  0x0000080010008080ULL, 0x0000040008008080ULL, 0x0000020004008080ULL,
  0x0000800100020080ULL, 0x0000800041000080ULL, 0x00FFFCDDFCED714AULL,
  0x007FFCDDFCED714AULL, 0x003FFFCDFFD88096ULL, 0x0000040810002101ULL,
  0x0001000204080011ULL, 0x0001000204000801ULL, 0x0001000082000401ULL,
  0x0001FFFAABFAD1A2ULL
};
BITBOARD magic_rook_mask[64] = {
  0x000101010101017EULL, 0x000202020202027CULL, 0x000404040404047AULL,
  0x0008080808080876ULL, 0x001010101010106EULL, 0x002020202020205EULL,
  0x004040404040403EULL, 0x008080808080807EULL, 0x0001010101017E00ULL,
  0x0002020202027C00ULL, 0x0004040404047A00ULL, 0x0008080808087600ULL,
  0x0010101010106E00ULL, 0x0020202020205E00ULL, 0x0040404040403E00ULL,
  0x0080808080807E00ULL, 0x00010101017E0100ULL, 0x00020202027C0200ULL,
  0x00040404047A0400ULL, 0x0008080808760800ULL, 0x00101010106E1000ULL,
  0x00202020205E2000ULL, 0x00404040403E4000ULL, 0x00808080807E8000ULL,
  0x000101017E010100ULL, 0x000202027C020200ULL, 0x000404047A040400ULL,
  0x0008080876080800ULL, 0x001010106E101000ULL, 0x002020205E202000ULL,
  0x004040403E404000ULL, 0x008080807E808000ULL, 0x0001017E01010100ULL,
  0x0002027C02020200ULL, 0x0004047A04040400ULL, 0x0008087608080800ULL,
  0x0010106E10101000ULL, 0x0020205E20202000ULL, 0x0040403E40404000ULL,
  0x0080807E80808000ULL, 0x00017E0101010100ULL, 0x00027C0202020200ULL,
  0x00047A0404040400ULL, 0x0008760808080800ULL, 0x00106E1010101000ULL,
  0x00205E2020202000ULL, 0x00403E4040404000ULL, 0x00807E8080808000ULL,
  0x007E010101010100ULL, 0x007C020202020200ULL, 0x007A040404040400ULL,
  0x0076080808080800ULL, 0x006E101010101000ULL, 0x005E202020202000ULL,
  0x003E404040404000ULL, 0x007E808080808000ULL, 0x7E01010101010100ULL,
  0x7C02020202020200ULL, 0x7A04040404040400ULL, 0x7608080808080800ULL,
  0x6E10101010101000ULL, 0x5E20202020202000ULL, 0x3E40404040404000ULL,
  0x7E80808080808000ULL
};
unsigned int magic_rook_shift[64] = {
  52, 53, 53, 53, 53, 53, 53, 52,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 53, 53, 53, 53, 53
};
BITBOARD mobility_mask_n[4] = {
  0xFF818181818181FFULL, 0x007E424242427E00ULL,
  0x00003C24243C0000ULL, 0x0000001818000000ULL
};
BITBOARD mobility_mask_b[4] = {
  0xFF818181818181FFULL, 0x007E424242427E00ULL,
  0x00003C24243C0000ULL, 0x0000001818000000ULL
};
BITBOARD mobility_mask_r[4] = {
  0x8181818181818181ULL, 0x4242424242424242ULL,
  0x2424242424242424ULL, 0x1818181818181818ULL
};
/*
  values use to deal with white/black independently
 */
const int rankflip[2][8] = {
  { RANK8, RANK7, RANK6, RANK5, RANK4, RANK3, RANK2, RANK1 },
  { RANK1, RANK2, RANK3, RANK4, RANK5, RANK6, RANK7, RANK8 }
};
const int sqflip[2][64] = {
 { A8, B8, C8, D8, E8, F8, G8, H8,
   A7, B7, C7, D7, E7, F7, G7, H7,
   A6, B6, C6, D6, E6, F6, G6, H6,
   A5, B5, C5, D5, E5, F5, G5, H5,
   A4, B4, C4, D4, E4, F4, G4, H4,   /* black */
   A3, B3, C3, D3, E3, F3, G3, H3,
   A2, B2, C2, D2, E2, F2, G2, H2,
   A1, B1, C1, D1, E1, F1, G1, H1 },

 { A1, B1, C1, D1, E1, F1, G1, H1,
   A2, B2, C2, D2, E2, F2, G2, H2,
   A3, B3, C3, D3, E3, F3, G3, H3,
   A4, B4, C4, D4, E4, F4, G4, H4,
   A5, B5, C5, D5, E5, F5, G5, H5,   /* white */
   A6, B6, C6, D6, E6, F6, G6, H6,
   A7, B7, C7, D7, E7, F7, G7, H7,
   A8, B8, C8, D8, E8, F8, G8, H8 }
};
const BITBOARD rank12[2] = { 0xffff000000000000ULL, 0x000000000000ffffULL };
const int sign[2] = { -1, 1 };
int direction[2] = { -8, 8 };
int dark_corner[2] = { FILEA, FILEH };
int light_corner[2] = { FILEH, FILEA };
int epsq[2] = { +8, -8 };
int rook_A[2] = { A8, A1 };
int rook_D[2] = { D8, D1 };
int rook_F[2] = { F8, F1 };
int rook_G[2] = { G8, G1 };
int rook_H[2] = { H8, H1 };
int pawnadv1[2] = { +8, -8 };
int pawnadv2[2] = { +16, -16 };
int capleft[2] = { +9, -7 };
int capright[2] = { +7, -9 };
const char empty_sqs[9] = { 0, '1', '2', '3', '4', '5', '6', '7', '8' };
/*
   This array is indexed by rook advantage and minor piece advantage.
   rook advantage is 4 + white rook equivalents - black rook equivalents 
   where a rook equivalent is number of rooks + 2 * number of queens.
   minor piece advantage is 4 + white minors - black minors.  

   The classic bad trade case is two minors for a rook.  If white trades
   two minors for a rook, rook advantage is +5 and minor advantage is +2.
   imbalance[5][2] gives a penalty of -42 for this trade.
*/
int imbalance[9][9] = {
/* n=-4  n=-3  n=-2  n=-1   n=0  n=+1  n=+2  n=+3 +n=+4 */
  {-126, -126, -126, -126, -126, -126, -126, -126,  -42 }, /* R=-4 */
  {-126, -126, -126, -126, -126, -126, -126,  -42,   42 }, /* R=-3 */
  {-126, -126, -126, -126, -126, -126,  -42,   42,   84 }, /* R=-2 */
  {-126, -126, -126, -126, -104,  -42,   42,   84,  126 }, /* R=-1 */
  {-126, -126, -126,  -88,    0,   88,  126,  126,  126 }, /*  R=0 */
  {-126,  -84,  -42,   42,  104,  126,  126,  126,  126 }, /* R=+1 */
  { -84,  -42,   42,  126,  126,  126,  126,  126,  126 }, /* R=+2 */
  { -42,   42,  126,  126,  126,  126,  126,  126,  126 }, /* R=+3 */
  {  42,  126,  126,  126,  126,  126,  126,  126,  126 }  /* R=+4 */
};
int pp_rank_bonus[8] = { 0, 0, 0, 2, 6, 12, 20, 0 };
int pp_dist_bonus[8] = { 0, 0, 0, 2, 7, 19, 31, 0 };
int pp_bonus[8] = { 0, 3, 3, 15, 35, 60, 100, 0 };
int king_tropism_n[8] = { 0, 3, 3, 2, 1, 0, 0, 0 };
int king_tropism_b[8] = { 0, 2, 2, 1, 0, 0, 0, 0 };
int king_tropism_r[8] = { 0, 4, 3, 2, 1, 1, 1, 1 };
int king_tropism_q[8] = { 0, 6, 5, 4, 3, 2, 2, 2 };
int passed_pawn_candidate[2][2][8] = {
 {{ 0,  0, 36, 16,  8,  4,  4, 0 },         /* [mg][black][rank] */
  { 0,  4,  4,  8, 16, 36,  0, 0 }},        /* [mg][white][rank] */
 {{ 0,  0, 48, 32, 16,  9,  9, 0 },         /* [eg][black][rank] */
  { 0,  9,  9, 16, 32, 48,  0, 0 }}         /* [eg][white][rank] */
};
int passed_pawn_value[2][2][8] = {
 {{ 0, 200, 120,  80,  20,   0,   0, 0 },   /* [mg][black][rank] */
  { 0,   0,   0,  20,  80, 120, 200, 0 }},  /* [mg][white][rank] */
 {{ 0, 234, 145,  95,  24,   2,   2, 0 },   /* [eg][black][rank] */
  { 0,   2,   2,  24,  95, 145, 234, 0 }}   /* [eg][white][rank] */
};
int connected_passed_pawn_value[2] = { 1, 3 };
int passed_pawn_hidden[2] = { 0, 40 };
int blockading_passed_pawn_value[2][2][8] = {
 {{ 0, 100, 60, 40, 10,  0,   0, 0 },       /* [mg][black][rank] */
  { 0,   0,  0, 10, 40, 60, 100, 0 }},      /* [mg][white][rank] */
 {{ 0, 100, 60, 40, 10,  0,   0, 0 },       /* [eg][black][rank] */
  { 0,   0,  0, 10, 40, 60, 100, 0 }}       /* [eg][white][rank] */
};
int doubled_pawn_value[2] = { 5, 6 };
int outside_passed[2] = { 20, 60 };
int pawn_defects[2][8] = {
  { 0, 0, 0, 1, 2, 3, 0, 0 },               /* [black][8] */
  { 0, 0, 3, 2, 1, 0, 0, 0 }                /* [white][8] */
};
const char square_color[64] = {
  1, 0, 1, 0, 1, 0, 1, 0,
  0, 1, 0, 1, 0, 1, 0, 1,
  1, 0, 1, 0, 1, 0, 1, 0,
  0, 1, 0, 1, 0, 1, 0, 1,
  1, 0, 1, 0, 1, 0, 1, 0,
  0, 1, 0, 1, 0, 1, 0, 1,
  1, 0, 1, 0, 1, 0, 1, 0,
  0, 1, 0, 1, 0, 1, 0, 1
};
const char b_n_mate_dark_squares[64] = {
  99, 90, 80, 70, 60, 50, 40, 30,
  90, 80, 70, 60, 50, 40, 30, 40,
  80, 70, 60, 50, 40, 30, 40, 50,
  70, 60, 50, 40, 30, 40, 50, 60,
  60, 50, 40, 30, 40, 50, 60, 70,
  50, 40, 30, 40, 50, 60, 70, 80,
  40, 30, 40, 50, 60, 70, 80, 90,
  30, 40, 50, 60, 70, 80, 90, 99
};
const char b_n_mate_light_squares[64] = {
  30, 40, 50, 60, 70, 80, 90, 99,
  40, 30, 40, 50, 60, 70, 80, 90,
  50, 40, 30, 40, 50, 60, 70, 80,
  60, 50, 40, 30, 40, 50, 60, 70,
  70, 60, 50, 40, 30, 40, 50, 60,
  80, 70, 60, 50, 40, 30, 40, 50,
  90, 80, 70, 60, 50, 40, 30, 40,
  99, 90, 80, 70, 60, 50, 40, 30
};
const int mate[64] = {
  200, 180, 160, 140, 140, 160, 180, 200,
  180, 160, 140, 120, 120, 140, 160, 180,
  160, 140, 120, 100, 100, 120, 140, 160,
  140, 120, 100, 100, 100, 100, 120, 140,
  140, 120, 100, 100, 100, 100, 120, 140,
  160, 140, 120, 100, 100, 120, 140, 160,
  180, 160, 140, 120, 120, 140, 160, 180,
  200, 180, 160, 140, 140, 160, 180, 200
};
int knight_outpost[2][64] = {
  { 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 4, 4, 4, 4, 1, 0,
    0, 2, 6, 8, 8, 6, 2, 0,
    0, 1, 4, 4, 4, 4, 1, 0,   /* [black][64] */
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0 },

  { 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 4, 4, 4, 4, 1, 0,
    0, 2, 6, 8, 8, 6, 2, 0,   /* [white][64] */
    0, 1, 4, 4, 4, 4, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0 }
};
int bishop_outpost[2][64] = {
  { 0, 0, 0, 0, 0, 0, 0, 0,
   -1, 0, 0, 0, 0, 0, 0,-1,
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 1, 3, 3, 3, 3, 1, 0,
    0, 3, 5, 5, 5, 5, 3, 0,   /* [black][64] */
    0, 1, 2, 2, 2, 2, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0 },

  { 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 2, 2, 2, 2, 1, 0,
    0, 3, 5, 5, 5, 5, 3, 0,
    0, 1, 3, 3, 3, 3, 1, 0,   /* [white][64] */
    0, 0, 1, 1, 1, 1, 0, 0,
   -1, 0, 0, 0, 0, 0, 0,-1,
    0, 0, 0, 0, 0, 0, 0, 0 }
};
int pval[2][2][64] = {
   {{ 0,   0,   0,   0,   0,   0,   0,   0,
     66,  66,  66,  66,  66,  66,  66,  66,
     10,  10,  10,  30,  30,  10,  10,  10,
      6,   6,   6,  16,  16,   6,   6,   6,
      3,   3,   3,  13,  13,   3,   3,   3,   /* [mg][black][64] */
      1,   1,   1,  10,  10,   1,   1,   1,
      0,   0,   0, -12, -12,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0 },

    { 0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0, -12, -12,   0,   0,   0,
      1,   1,   1,  10,  10,   1,   1,   1,
      3,   3,   3,  13,  13,   3,   3,   3,
      6,   6,   6,  16,  16,   6,   6,   6,   /* [mg][white][64] */
     10,  10,  10,  30,  30,  10,  10,  10,
     66,  66,  66,  66,  66,  66,  66,  66,
      0,   0,   0,   0,   0,   0,   0,   0 }},

   {{ 0,   0,   0,   0,   0,   0,   0,   0,
     66,  66,  66,  66,  66,  66,  66,  66,
     10,  10,  10,  30,  30,  10,  10,  10,
      6,   6,   6,  16,  16,   6,   6,   6,
      3,   3,   3,  13,  13,   3,   3,   3,   /* [eg][black][64] */
      1,   1,   1,  10,  10,   1,   1,   1,
      0,   0,   0, -12, -12,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0 },

    { 0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0, -12, -12,   0,   0,   0,
      1,   1,   1,  10,  10,   1,   1,   1,
      3,   3,   3,  13,  13,   3,   3,   3,
      6,   6,   6,  16,  16,   6,   6,   6,   /* [eg][white][64] */
     10,  10,  10,  30,  30,  10,  10,  10,
     66,  66,  66,  66,  66,  66,  66,  66,
      0,   0,   0,   0,   0,   0,   0,   0 }}
};
int nval[2][2][64] = {
  {{-29, -19, -19,  -9,  -9, -19, -19, -29,
      1,  12,  18,  22,  22,  18,  12,   1,
      1,  14,  23,  27,  27,  23,  14,   1,
      1,  14,  23,  28,  28,  23,  14,   1,
      1,  12,  21,  24,  24,  21,  12,   1,  /* [mg][black][64] */
      1,   2,  19,  17,  17,  19,   2,   1,
      1,   2,   2,   2,   2,   2,   2,   1,
    -19, -19, -19, -19, -19, -19, -19, -19 },

   {-19, -19, -19, -19, -19, -19, -19, -19,
      1,   2,   2,   2,   2,   2,   2,   1,
      1,   2,  19,  17,  17,  19,   2,   1,
      1,  12,  21,  24,  24,  21,  12,   1,
      1,  14,  23,  28,  28,  23,  14,   1,  /* [mg][white][64] */
      1,  14,  23,  27,  27,  23,  14,   1,
      1,  12,  18,  22,  22,  18,  12,   1,
    -29, -19, -19,  -9,  -9, -19, -19, -29 }},

  {{-29, -19, -19,  -9,  -9, -19, -19, -29,
      1,  12,  18,  22,  22,  18,  12,   1,
      1,  14,  23,  27,  27,  23,  14,   1,
      1,  14,  23,  28,  28,  23,  14,   1,
      1,  12,  21,  24,  24,  21,  12,   1,  /* [eg][black][64] */
      1,   2,  19,  17,  17,  19,   2,   1,
      1,   2,   2,   2,   2,   2,   2,   1,
    -19, -19, -19, -19, -19, -19, -19, -19 },

   {-19, -19, -19, -19, -19, -19, -19, -19,
      1,   2,   2,   2,   2,   2,   2,   1,
      1,   2,  19,  17,  17,  19,   2,   1,
      1,  12,  21,  24,  24,  21,  12,   1,
      1,  14,  23,  28,  28,  23,  14,   1,  /* [eg][white][64] */
      1,  14,  23,  27,  27,  23,  14,   1,
      1,  12,  18,  22,  22,  18,  12,   1,
    -29, -19, -19,  -9,  -9, -19, -19, -29 }}
};
int bval[2][2][64] = {
  {{  0,   0,   2,   4,   4,   2,   0,   0,
      0,   8,   6,   8,   8,   6,   8,   0,
      2,   6,  12,  10,  10,  12,   6,   2,
      4,   8,  10,  16,  16,  10,   8,   4,
      4,   8,  10,  16,  16,  10,   8,   4,   /* [mg][black][64] */
      2,   6,  12,  10,  10,  12,   6,   2,
      0,   8,   6,   8,   8,   6,   8,   0,
    -10, -10,  -8,  -6,  -6,  -8, -10, -10 },

   {-10, -10,  -8,  -6,  -6,  -8, -10, -10,
      0,   8,   6,   8,   8,   6,   8,   0,
      2,   6,  12,  10,  10,  12,   6,   2,
      4,   8,  10,  16,  16,  10,   8,   4,
      4,   8,  10,  16,  16,  10,   8,   4,   /* [mg][white][64] */
      2,   6,  12,  10,  10,  12,   6,   2,
      0,   8,   6,   8,   8,   6,   8,   0,
      0,   0,   2,   4,   4,   2,   0,   0 }},

  {{  0,   0,   2,   4,   4,   2,   0,   0,
      0,   8,   6,   8,   8,   6,   8,   0,
      2,   6,  12,  10,  10,  12,   6,   2,
      4,   8,  10,  16,  16,  10,   8,   4,
      4,   8,  10,  16,  16,  10,   8,   4,   /* [eg][black][64] */
      2,   6,  12,  10,  10,  12,   6,   2,
      0,   8,   6,   8,   8,   6,   8,   0,
    -10, -10,  -8,  -6,  -6,  -8, -10, -10 },

   {-10, -10,  -8,  -6,  -6,  -8, -10, -10,
      0,   8,   6,   8,   8,   6,   8,   0,
      2,   6,  12,  10,  10,  12,   6,   2,
      4,   8,  10,  16,  16,  10,   8,   4,
      4,   8,  10,  16,  16,  10,   8,   4,   /* [eg][white][64] */
      2,   6,  12,  10,  10,  12,   6,   2,
      0,   8,   6,   8,   8,   6,   8,   0,
      0,   0,   2,   4,   4,   2,   0,   0 }}
};
int qval[2][2][64] = {
   {{ 0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   4,   4,   4,   4,   0,   0,
      0,   4,   4,   6,   6,   4,   4,   0,
      0,   4,   6,   8,   8,   6,   4,   0,
      0,   4,   6,   8,   8,   6,   4,   0,   /* [mg][black][64] */
      0,   4,   4,   6,   6,   4,   4,   0,
      0,   0,   4,   4,   4,   4,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0 },

    { 0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   4,   4,   4,   4,   0,   0,
      0,   4,   4,   6,   6,   4,   4,   0,
      0,   4,   6,   8,   8,   6,   4,   0,
      0,   4,   6,   8,   8,   6,   4,   0,   /* [mg][white][64] */
      0,   4,   4,   6,   6,   4,   4,   0,
      0,   0,   4,   4,   4,   4,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0 }},

   {{ 0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   4,   4,   4,   4,   0,   0,
      0,   4,   4,   6,   6,   4,   4,   0,
      0,   4,   6,   8,   8,   6,   4,   0,
      0,   4,   6,   8,   8,   6,   4,   0,   /* [eg][black][64] */
      0,   4,   4,   6,   6,   4,   4,   0,
      0,   0,   4,   4,   4,   4,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0 },

    { 0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   4,   4,   4,   4,   0,   0,
      0,   4,   4,   6,   6,   4,   4,   0,
      0,   4,   6,   8,   8,   6,   4,   0,
      0,   4,   6,   8,   8,   6,   4,   0,   /* [eg][white][64] */
      0,   4,   4,   6,   6,   4,   4,   0,
      0,   0,   4,   4,   4,   4,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0 }}
};
int kval_n[2][64] = {
   {-40, -40, -40, -40, -40, -40, -40, -40,
    -40, -10, -10, -10, -10, -10, -10, -40,
    -40, -10,  60,  60,  60,  60, -10, -40,
    -40, -10,  60,  60,  60,  60, -10, -40,
    -40, -10,  40,  40,  40,  40, -10, -40,   /* [black][64] */
    -40, -10,  20,  20,  20,  20, -10, -40,
    -40, -10, -10, -10, -10, -10, -10, -40,
    -40, -40, -40, -40, -40, -40, -40, -40 },

   {-40, -40, -40, -40, -40, -40, -40, -40,
    -40, -10, -10, -10, -10, -10, -10, -40,
    -40, -10,  20,  20,  20,  20, -10, -40,
    -40, -10,  40,  40,  40,  40, -10, -40,
    -40, -10,  60,  60,  60,  60, -10, -40,   /* [white][64] */
    -40, -10,  60,  60,  60,  60, -10, -40,
    -40, -10, -10, -10, -10, -10, -10, -40,
    -40, -40, -40, -40, -40, -40, -40, -40 }
};
int kval_k[2][64] = {
   {-60, -40, -20, -20, -20, -20, -20, -20,
    -60, -40, -20,  20,  40,  40,  40,  40,
    -60, -40, -20,  20,  60,  60,  60,  40,
    -60, -40, -20,  20,  60,  60,  60,  40,
    -60, -40, -20,  20,  40,  40,  40,  40,   /* [black][64] */
    -60, -40, -20,  20,  20,  20,  20,  20,
    -60, -40, -20,   0,   0,   0,   0,   0,
    -60, -40, -20, -20, -20, -20, -20, -20 },

   {-60, -40, -20, -20, -20, -20, -20, -20,
    -60, -40, -20,   0,   0,   0,   0,   0,
    -60, -40, -20,  20,  20,  20,  20,  20,
    -60, -40, -20,  20,  40,  40,  40,  40,
    -60, -40, -20,  20,  60,  60,  60,  40,   /* [white][64] */
    -60, -40, -20,  20,  60,  60,  60,  40,
    -60, -40, -20,  20,  40,  40,  40,  40,
    -60, -40, -20, -20, -20, -20, -20, -20 }
};
int kval_q[2][64] = {
   {-20, -20, -20, -20, -20, -20, -40, -60,
     40,  40,  40,  40,  20, -20, -40, -60,
     40,  60,  60,  60,  20, -20, -40, -60,
     40,  60,  60,  60,  20, -20, -40, -60,
     40,  40,  40,  40,  20, -20, -40, -60,   /* [black][64] */
     20,  20,  20,  20,  20, -20, -40, -60,
      0,   0,   0,   0,   0, -20, -40, -60,
    -20, -20, -20, -20, -20, -20, -40, -60 },

   {-20, -20, -20, -20, -20, -20, -40, -60,
      0,   0,   0,   0,   0, -20, -40, -60,
     20,  20,  20,  20,  20, -20, -40, -60,
     40,  40,  40,  40,  20, -20, -40, -60,
     40,  60,  60,  60,  20, -20, -40, -60,   /* [white][64] */
     40,  60,  60,  60,  20, -20, -40, -60,
     40,  40,  40,  40,  20, -20, -40, -60,
    -20, -20, -20, -20, -20, -20, -40, -60 }
};
int safety_vector[16] = {
   0,  7, 14, 21, 28, 35, 42,  49,
  56, 63, 70, 77, 84, 91, 98, 105
};
int tropism_vector[16] = {
   0,  1,  2,  3,   4,   5,  11,  20,
  32, 47, 65, 86, 110, 137, 167, 200
};
const int p_values[13] = { 10000, 900, 500, 300, 300, 100, 0,
  100, 300, 300, 500, 900, 9900
};
const int pc_values[7] = { 0, 100, 300, 300, 500, 900, 9900 };
const int p_vals[7] = { 0, 1, 3, 3, 5, 9, 99 };
const int pieces[2][7] = {
  { 0, -1, -2, -3, -4, -5, -6 },
  { 0, +1, +2, +3, +4, +5, +6 }
};
int pawn_value = PAWN_VALUE;
int knight_value = KNIGHT_VALUE;
int bishop_value = BISHOP_VALUE;
int rook_value = ROOK_VALUE;
int queen_value = QUEEN_VALUE;
int king_value = KING_VALUE;
int piece_values[7][2] = { {0, 0},
  {-PAWN_VALUE, PAWN_VALUE},     {-KNIGHT_VALUE, KNIGHT_VALUE},
  {-BISHOP_VALUE, BISHOP_VALUE}, {-ROOK_VALUE, ROOK_VALUE},
  {-QUEEN_VALUE, QUEEN_VALUE},   {-KING_VALUE, KING_VALUE}
};
int pawn_can_promote = 525;
int bad_trade = 90;
int wtm_bonus[2] = { 5, 8 };
int undeveloped_piece = 12;
int pawn_duo[2] = { 4, 8 };
int pawn_isolated[2] = { 18, 21 };
int pawn_weak[2] = { 8, 18 };
int lower_n = 16;
int mobility_score_n[4] = { 1, 2, 3, 4 };
int lower_b = 10;
int bishop_pair[2] = {30, 30};
int bishop_trapped = 174;
int bishop_with_wing_pawns[2] = { 18, 36 };
int mobility_score_b[4] = { 1, 2, 3, 4 };
int lower_r = 16;
int mobility_score_r[4] = { 1, 2, 3, 4 };
int rook_on_7th[2] = { 25, 35 };
int rook_open_file[2] = { 35, 20 };
int rook_half_open_file[2] = { 10, 10 };
int rook_behind_passed_pawn[2] = { 10, 36 };
int rook_trapped = 60;
int open_file[8] = { 6, 5, 4, 4, 4, 4, 5, 6 };
int half_open_file[8] = { 4, 4, 3, 3, 3, 3, 4, 4 };
int king_safety_mate_threat = 600;
int king_king_tropism = 10;
int development_thematic = 12;
int development_losing_castle = 20;
int development_not_castled = 20;
/*
   First term is a character string explaining what the eval
   term is used for.

  Second term is the "type" of the value.  
     0 = heading entry (no values, just a category name to display).
     1 = scalar value.
     2 = mg/eg two-element array
     3 = array[mg][side][64].  These values are displayed as 8x8
       boards with white values at the bottom, and the ranks/files
       labeled to make them easier to read.
     4 = array[side][64]
     5 = array[side][N!=64]
     6 = array[mg][side][8]
     7 = array[mg][small#]
     8 = array[n]
     9 = btm/wtm two element array

   Third term is the "size" of the scoring term, where 0 is a
     scalar value, otherwise it is the actual number of elements in
     an array of values.

   Fourth term is a pointer to the data value(s).
*/
struct personality_term personality_packet[256] = {
  {"search options                       ", 0, 0, NULL},        /* 0 */
  {"check extension                      ", 1, 0, &check_depth},
  {"null-move reduction                  ", 1, 0, &null_depth},
  {"LMR min distance to frontier         ", 1, 0, &LMR_remaining_depth},
  {"LMR min reduction                    ", 1, 0, &LMR_min_reduction},
  {"LMR max reduction                    ", 1, 0, &LMR_max_reduction},
  {"prune depth                          ", 1, 0, &pruning_depth},
  {"prune margin                         ", 8, 8, pruning_margin},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {"raw piece values                     ", 0, 0, NULL},        /* 10 */
  {"pawn value                           ", 9, 2, piece_values[pawn]},
  {"knight value                         ", 9, 2, piece_values[knight]},
  {"bishop value                         ", 9, 2, piece_values[bishop]},
  {"rook value                           ", 9, 2, piece_values[rook]},
  {"queen value                          ", 9, 2, piece_values[queen]},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {"miscellaneous scoring values         ", 0, 0, NULL},        /* 20 */
  {"bad trade bonus/penalty              ", 1, 0, &bad_trade},
  {"wtm bonus                            ", 2, 2, wtm_bonus},
  {"draw score                           ", 1, 0, &abs_draw_score},
#if defined(SKILL)
  {"skill level setting                  ", 1, 0, &skill},
#else
  {NULL, 0, 0, NULL},
#endif
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {"pawn evaluation                      ", 0, 0, NULL},        /* 30 */
  {"pawn piece/square table (white)      ", 3, 256, (int *) pval},
  {"connected passed pawn [rank]         ", 2, 2, connected_passed_pawn_value},
  {"passed pawn [rank]                   ", 6, 32, (int *) passed_pawn_value},
  {"blockading a passed pawn [rank]      ", 6, 32, (int *) blockading_passed_pawn_value},
  {"pawn duo                             ", 2, 2, pawn_duo},
  {"pawn isolated                        ", 2, 2, pawn_isolated},
  {"pawn weak                            ", 2, 2, pawn_weak},
  {"pawn can promote                     ", 1, 0, &pawn_can_promote},
  {"outside passed pawn                  ", 2, 2, outside_passed},
  {"hidden passed pawn                   ", 2, 2, passed_pawn_hidden},
  {"doubled pawn                         ", 2, 2, doubled_pawn_value},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {"knight scoring                       ", 0, 0, NULL},        /* 50 */
  {"knight piece/square table (white)    ", 3, 256, (int *) nval},
  {"knight outpost [square]              ", 4, 128, (int *) knight_outpost},
  {"knight king tropism [distance]       ", 8, 8, king_tropism_n},
  {"knight mobility                      ", 8, 4, mobility_score_n},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {"bishop scoring                       ", 0, 0, NULL},        /* 60 */
  {"bishop pair                          ", 2, 2, bishop_pair},
  {"bishop piece/square table (white)    ", 3, 256, (int *) bval},
  {"bishop king tropism [distance]       ", 8, 8, king_tropism_b},
  {"bishop mobility/square table         ", 8, 4, (int *) mobility_score_b},
  {"bishop with wing pawns               ", 2, 2, bishop_with_wing_pawns},
  {"bishop trapped                       ", 1, 0, &bishop_trapped},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {"rook scoring                         ", 0, 0, NULL},        /* 70 */
  {"rook open file                       ", 2, 2, rook_open_file},
  {"rook king tropism [distance]         ", 8, 8, king_tropism_r},
  {"rook mobility/square table           ", 8, 4, mobility_score_r},
  {"rook half open file                  ", 2, 2, rook_half_open_file},
  {"rook on 7th                          ", 2, 2, rook_on_7th},
  {"rook behind passed pawn              ", 2, 2, rook_behind_passed_pawn},
  {"rook trapped                         ", 1, 0, &rook_trapped},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {"queen scoring                        ", 0, 0, NULL},        /* 80 */
  {"queen piece/square table (white)     ", 3, 256, (int *) qval},
  {"queen king tropism [distance]        ", 8, 8, king_tropism_q},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {"king scoring                         ", 0, 0, NULL},        /* 90 */
  {"king piece/square normal             ", 4, 128, (int *)kval_n},
  {"king piece/square kside pawns        ", 4, 128, (int *)kval_k},
  {"king piece/square qside pawns        ", 4, 128, (int *)kval_q},
  {"king safety pawn-shield vector       ", 8, 16, safety_vector},
  {"king safety tropism vector           ", 8, 16, tropism_vector},
  {"king safe open file [file]           ", 8, 8, open_file},
  {"king safe half-open file [file]      ", 8, 8, half_open_file},
  {"king king tropism (endgame)          ", 1, 0, &king_king_tropism},
  {"king safety trojan horse threat      ", 1, 0, &king_safety_mate_threat},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {"development scoring                  ", 0, 0, NULL},        /* 110 */
  {"development thematic                 ", 1, 0, &development_thematic},
  {"development losing castle rights     ", 1, 0, &development_losing_castle},
  {"development not castled              ", 1, 0, &development_not_castled},
};
/* *INDENT-ON* */
