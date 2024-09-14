/*
 * (C) 2011- TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ttxssh.h"
#include "kex.h"


char *myproposal[PROPOSAL_MAX] = {
	KEX_DEFAULT_KEX,
	KEX_DEFAULT_PK_ALG,
	KEX_DEFAULT_ENCRYPT,
	KEX_DEFAULT_ENCRYPT,
	KEX_DEFAULT_MAC,
	KEX_DEFAULT_MAC,
	KEX_DEFAULT_COMP,
	KEX_DEFAULT_COMP,
	KEX_DEFAULT_LANG,
	KEX_DEFAULT_LANG,
};

struct ssh2_kex_algorithm_t {
	kex_algorithm kextype;
	const char *name;
	const EVP_MD *(*evp_md)(void);
};

static const struct ssh2_kex_algorithm_t ssh2_kex_algorithms[] = {
	{KEX_DH_GRP1_SHA1,  "diffie-hellman-group1-sha1",           EVP_sha1},   // RFC4253
	{KEX_DH_GRP14_SHA1, "diffie-hellman-group14-sha1",          EVP_sha1},   // RFC4253
	{KEX_DH_GEX_SHA1,   "diffie-hellman-group-exchange-sha1",   EVP_sha1},   // RFC4419
	{KEX_DH_GEX_SHA256, "diffie-hellman-group-exchange-sha256", EVP_sha256}, // RFC4419
	{KEX_ECDH_SHA2_256, "ecdh-sha2-nistp256",                   EVP_sha256}, // RFC5656
	{KEX_ECDH_SHA2_384, "ecdh-sha2-nistp384",                   EVP_sha384}, // RFC5656
	{KEX_ECDH_SHA2_521, "ecdh-sha2-nistp521",                   EVP_sha512}, // RFC5656
	{KEX_DH_GRP14_SHA256, "diffie-hellman-group14-sha256",      EVP_sha256}, // RFC8268
	{KEX_DH_GRP16_SHA512, "diffie-hellman-group16-sha512",      EVP_sha512}, // RFC8268
	{KEX_DH_GRP18_SHA512, "diffie-hellman-group18-sha512",      EVP_sha512}, // RFC8268
	{KEX_DH_NONE      , NULL,                                   NULL},
};


const char* get_kex_algorithm_name(kex_algorithm kextype)
{
	const struct ssh2_kex_algorithm_t *ptr = ssh2_kex_algorithms;

	while (ptr->name != NULL) {
		if (kextype == ptr->kextype) {
			return ptr->name;
		}
		ptr++;
	}

	// not found.
	return "unknown";
}

const EVP_MD* get_kex_algorithm_EVP_MD(kex_algorithm kextype)
{
	const struct ssh2_kex_algorithm_t *ptr = ssh2_kex_algorithms;

	while (ptr->name != NULL) {
		if (kextype == ptr->kextype) {
			return ptr->evp_md();
		}
		ptr++;
	}

	// not found.
	return EVP_md_null();
}

void normalize_kex_order(char *buf)
{
	static char default_strings[] = {
		KEX_ECDH_SHA2_256,
		KEX_ECDH_SHA2_384,
		KEX_ECDH_SHA2_521,
		KEX_DH_GRP18_SHA512,
		KEX_DH_GRP16_SHA512,
		KEX_DH_GRP14_SHA256,
		KEX_DH_GEX_SHA256,
		KEX_DH_GRP14_SHA1,
		KEX_DH_NONE,
		KEX_DH_GEX_SHA1,
		KEX_DH_GRP1_SHA1,
	};

	normalize_generic_order(buf, default_strings, NUM_ELEM(default_strings));
}

kex_algorithm choose_SSH2_kex_algorithm(char *server_proposal, char *my_proposal)
{
	kex_algorithm type = KEX_DH_UNKNOWN;
	char str_kextype[40];
	const struct ssh2_kex_algorithm_t *ptr = ssh2_kex_algorithms;

	choose_SSH2_proposal(server_proposal, my_proposal, str_kextype, sizeof(str_kextype));

	while (ptr->name != NULL) {
		if (strcmp(ptr->name, str_kextype) == 0) {
			type = ptr->kextype;
			break;
		}
		ptr++;
	}

	return (type);
}

// KEX�A���S���Y���D�揇�ʂɉ����āAmyproposal[]������������B
// (2011.2.28 yutaka)
void SSH2_update_kex_myproposal(PTInstVar pvar)
{
	static char buf[512]; // TODO: malloc()�ɂ��ׂ�
	int index;
	int i;

	// �ʐM���ɌĂ΂��Ƃ������Ƃ̓L�[�č쐬
	if (pvar->socket != INVALID_SOCKET) {
		if (pvar->kex_status & KEX_FLAG_REKEYING) {
			// �L�[�č쐬�̏ꍇ�ɂ́A�ڑ����� pvar->settings ����g�ݗ��Ă�ꂽ myproposal ������������B
			//   pvar->settings �� �ڑ����� myproposal ���쐬�����Ƃ��̒l����ς���Ă��Ȃ��ۏ؂��Ȃ��B
			//   �ēx�g�ݗ��Ă�̂ł͂Ȃ������� myproposal �����������邱�Ƃɂ����B
			int pos = strlen(myproposal[PROPOSAL_KEX_ALGS]) - strlen(",ext-info-c,kex-strict-c-v00@openssh.com");
			if (strcmp(myproposal[PROPOSAL_KEX_ALGS] + pos, ",ext-info-c,kex-strict-c-v00@openssh.com") == 0) {
				myproposal[PROPOSAL_KEX_ALGS][pos] = '\0';
			}
		}
		return;
	}

	buf[0] = '\0';
	for (i = 0 ; pvar->settings.KexOrder[i] != 0 ; i++) {
		index = pvar->settings.KexOrder[i] - '0';
		if (index == KEX_DH_NONE) // disabled line
			break;
		strncat_s(buf, sizeof(buf), get_kex_algorithm_name(index), _TRUNCATE);
		strncat_s(buf, sizeof(buf), ",", _TRUNCATE);
	}

	// Enables RFC 8308 Extension Negotiation & Strict KEX mode (for CVE-2023-48795)
	strncat_s(buf, sizeof(buf), "ext-info-c,kex-strict-c-v00@openssh.com", _TRUNCATE);

	myproposal[PROPOSAL_KEX_ALGS] = buf;
}


static DH *dh_new_group_asc(const char *gen, const char *modulus)
{
	DH *dh = NULL;
	BIGNUM *p = NULL, *g = NULL;

	if ((dh = DH_new()) == NULL) {
		printf("dh_new_group_asc: DH_new");
		goto error;
	}

	// P��G�͌��J���Ă��悢�f���̑g�ݍ��킹
	if (BN_hex2bn(&p, modulus) == 0) {
		printf("BN_hex2bn p");
		goto error;
	}

	if (BN_hex2bn(&g, gen) == 0) {
		printf("BN_hex2bn g");
		goto error;
	}

	// BN_hex2bn()�ŕϊ������|�C���^��DH�\���̂ɃZ�b�g����B
	DH_set0_pqg(dh, p, NULL, g);

	return (dh);

error:
    BN_free(g);
    BN_free(p);
	DH_free(dh);
	return (NULL);
}


DH *dh_new_group1(void)
{
	static char *gen = "2", *group1 =
	    "FFFFFFFF" "FFFFFFFF" "C90FDAA2" "2168C234" "C4C6628B" "80DC1CD1"
	    "29024E08" "8A67CC74" "020BBEA6" "3B139B22" "514A0879" "8E3404DD"
	    "EF9519B3" "CD3A431B" "302B0A6D" "F25F1437" "4FE1356D" "6D51C245"
	    "E485B576" "625E7EC6" "F44C42E9" "A637ED6B" "0BFF5CB6" "F406B7ED"
	    "EE386BFB" "5A899FA5" "AE9F2411" "7C4B1FE6" "49286651" "ECE65381"
	    "FFFFFFFF" "FFFFFFFF";

	return (dh_new_group_asc(gen, group1));
}


DH *dh_new_group14(void)
{
    static char *gen = "2", *group14 =
        "FFFFFFFF" "FFFFFFFF" "C90FDAA2" "2168C234" "C4C6628B" "80DC1CD1"
        "29024E08" "8A67CC74" "020BBEA6" "3B139B22" "514A0879" "8E3404DD"
        "EF9519B3" "CD3A431B" "302B0A6D" "F25F1437" "4FE1356D" "6D51C245"
        "E485B576" "625E7EC6" "F44C42E9" "A637ED6B" "0BFF5CB6" "F406B7ED"
        "EE386BFB" "5A899FA5" "AE9F2411" "7C4B1FE6" "49286651" "ECE45B3D"
        "C2007CB8" "A163BF05" "98DA4836" "1C55D39A" "69163FA8" "FD24CF5F"
        "83655D23" "DCA3AD96" "1C62F356" "208552BB" "9ED52907" "7096966D"
        "670C354E" "4ABC9804" "F1746C08" "CA18217C" "32905E46" "2E36CE3B"
        "E39E772C" "180E8603" "9B2783A2" "EC07A28F" "B5C55DF0" "6F4C52C9"
        "DE2BCBF6" "95581718" "3995497C" "EA956AE5" "15D22618" "98FA0510"
        "15728E5A" "8AACAA68" "FFFFFFFF" "FFFFFFFF";

	return (dh_new_group_asc(gen, group14));
}

// ���g�p
DH *dh_new_group15(void)
{
    static char *gen = "2", *group15 =
	"FFFFFFFF" "FFFFFFFF" "C90FDAA2" "2168C234" "C4C6628B" "80DC1CD1"
	"29024E08" "8A67CC74" "020BBEA6" "3B139B22" "514A0879" "8E3404DD"
	"EF9519B3" "CD3A431B" "302B0A6D" "F25F1437" "4FE1356D" "6D51C245"
	"E485B576" "625E7EC6" "F44C42E9" "A637ED6B" "0BFF5CB6" "F406B7ED"
	"EE386BFB" "5A899FA5" "AE9F2411" "7C4B1FE6" "49286651" "ECE45B3D"
	"C2007CB8" "A163BF05" "98DA4836" "1C55D39A" "69163FA8" "FD24CF5F"
	"83655D23" "DCA3AD96" "1C62F356" "208552BB" "9ED52907" "7096966D"
	"670C354E" "4ABC9804" "F1746C08" "CA18217C" "32905E46" "2E36CE3B"
	"E39E772C" "180E8603" "9B2783A2" "EC07A28F" "B5C55DF0" "6F4C52C9"
	"DE2BCBF6" "95581718" "3995497C" "EA956AE5" "15D22618" "98FA0510"
	"15728E5A" "8AAAC42D" "AD33170D" "04507A33" "A85521AB" "DF1CBA64"
	"ECFB8504" "58DBEF0A" "8AEA7157" "5D060C7D" "B3970F85" "A6E1E4C7"
	"ABF5AE8C" "DB0933D7" "1E8C94E0" "4A25619D" "CEE3D226" "1AD2EE6B"
	"F12FFA06" "D98A0864" "D8760273" "3EC86A64" "521F2B18" "177B200C"
	"BBE11757" "7A615D6C" "770988C0" "BAD946E2" "08E24FA0" "74E5AB31"
	"43DB5BFC" "E0FD108E" "4B82D120" "A93AD2CA" "FFFFFFFF" "FFFFFFFF";
	return (dh_new_group_asc(gen, group15));
}

DH *dh_new_group16(void)
{
    static char *gen = "2", *group16 =
	"FFFFFFFF" "FFFFFFFF" "C90FDAA2" "2168C234" "C4C6628B" "80DC1CD1"
	"29024E08" "8A67CC74" "020BBEA6" "3B139B22" "514A0879" "8E3404DD"
	"EF9519B3" "CD3A431B" "302B0A6D" "F25F1437" "4FE1356D" "6D51C245"
	"E485B576" "625E7EC6" "F44C42E9" "A637ED6B" "0BFF5CB6" "F406B7ED"
	"EE386BFB" "5A899FA5" "AE9F2411" "7C4B1FE6" "49286651" "ECE45B3D"
	"C2007CB8" "A163BF05" "98DA4836" "1C55D39A" "69163FA8" "FD24CF5F"
	"83655D23" "DCA3AD96" "1C62F356" "208552BB" "9ED52907" "7096966D"
	"670C354E" "4ABC9804" "F1746C08" "CA18217C" "32905E46" "2E36CE3B"
	"E39E772C" "180E8603" "9B2783A2" "EC07A28F" "B5C55DF0" "6F4C52C9"
	"DE2BCBF6" "95581718" "3995497C" "EA956AE5" "15D22618" "98FA0510"
	"15728E5A" "8AAAC42D" "AD33170D" "04507A33" "A85521AB" "DF1CBA64"
	"ECFB8504" "58DBEF0A" "8AEA7157" "5D060C7D" "B3970F85" "A6E1E4C7"
	"ABF5AE8C" "DB0933D7" "1E8C94E0" "4A25619D" "CEE3D226" "1AD2EE6B"
	"F12FFA06" "D98A0864" "D8760273" "3EC86A64" "521F2B18" "177B200C"
	"BBE11757" "7A615D6C" "770988C0" "BAD946E2" "08E24FA0" "74E5AB31"
	"43DB5BFC" "E0FD108E" "4B82D120" "A9210801" "1A723C12" "A787E6D7"
	"88719A10" "BDBA5B26" "99C32718" "6AF4E23C" "1A946834" "B6150BDA"
	"2583E9CA" "2AD44CE8" "DBBBC2DB" "04DE8EF9" "2E8EFC14" "1FBECAA6"
	"287C5947" "4E6BC05D" "99B2964F" "A090C3A2" "233BA186" "515BE7ED"
	"1F612970" "CEE2D7AF" "B81BDD76" "2170481C" "D0069127" "D5B05AA9"
	"93B4EA98" "8D8FDDC1" "86FFB7DC" "90A6C08F" "4DF435C9" "34063199"
	"FFFFFFFF" "FFFFFFFF";
	return (dh_new_group_asc(gen, group16));
}

// ���g�p
DH *dh_new_group17(void)
{
    static char *gen = "2", *group17 =
	"FFFFFFFF" "FFFFFFFF" "C90FDAA2" "2168C234" "C4C6628B" "80DC1CD1" "29024E08"
	"8A67CC74" "020BBEA6" "3B139B22" "514A0879" "8E3404DD" "EF9519B3" "CD3A431B"
	"302B0A6D" "F25F1437" "4FE1356D" "6D51C245" "E485B576" "625E7EC6" "F44C42E9"
	"A637ED6B" "0BFF5CB6" "F406B7ED" "EE386BFB" "5A899FA5" "AE9F2411" "7C4B1FE6"
	"49286651" "ECE45B3D" "C2007CB8" "A163BF05" "98DA4836" "1C55D39A" "69163FA8"
	"FD24CF5F" "83655D23" "DCA3AD96" "1C62F356" "208552BB" "9ED52907" "7096966D"
	"670C354E" "4ABC9804" "F1746C08" "CA18217C" "32905E46" "2E36CE3B" "E39E772C"
	"180E8603" "9B2783A2" "EC07A28F" "B5C55DF0" "6F4C52C9" "DE2BCBF6" "95581718"
	"3995497C" "EA956AE5" "15D22618" "98FA0510" "15728E5A" "8AAAC42D" "AD33170D"
	"04507A33" "A85521AB" "DF1CBA64" "ECFB8504" "58DBEF0A" "8AEA7157" "5D060C7D"
	"B3970F85" "A6E1E4C7" "ABF5AE8C" "DB0933D7" "1E8C94E0" "4A25619D" "CEE3D226"
	"1AD2EE6B" "F12FFA06" "D98A0864" "D8760273" "3EC86A64" "521F2B18" "177B200C"
	"BBE11757" "7A615D6C" "770988C0" "BAD946E2" "08E24FA0" "74E5AB31" "43DB5BFC"
	"E0FD108E" "4B82D120" "A9210801" "1A723C12" "A787E6D7" "88719A10" "BDBA5B26"
	"99C32718" "6AF4E23C" "1A946834" "B6150BDA" "2583E9CA" "2AD44CE8" "DBBBC2DB"
	"04DE8EF9" "2E8EFC14" "1FBECAA6" "287C5947" "4E6BC05D" "99B2964F" "A090C3A2"
	"233BA186" "515BE7ED" "1F612970" "CEE2D7AF" "B81BDD76" "2170481C" "D0069127"
	"D5B05AA9" "93B4EA98" "8D8FDDC1" "86FFB7DC" "90A6C08F" "4DF435C9" "34028492"
	"36C3FAB4" "D27C7026" "C1D4DCB2" "602646DE" "C9751E76" "3DBA37BD" "F8FF9406"
	"AD9E530E" "E5DB382F" "413001AE" "B06A53ED" "9027D831" "179727B0" "865A8918"
	"DA3EDBEB" "CF9B14ED" "44CE6CBA" "CED4BB1B" "DB7F1447" "E6CC254B" "33205151"
	"2BD7AF42" "6FB8F401" "378CD2BF" "5983CA01" "C64B92EC" "F032EA15" "D1721D03"
	"F482D7CE" "6E74FEF6" "D55E702F" "46980C82" "B5A84031" "900B1C9E" "59E7C97F"
	"BEC7E8F3" "23A97A7E" "36CC88BE" "0F1D45B7" "FF585AC5" "4BD407B2" "2B4154AA"
	"CC8F6D7E" "BF48E1D8" "14CC5ED2" "0F8037E0" "A79715EE" "F29BE328" "06A1D58B"
	"B7C5DA76" "F550AA3D" "8A1FBFF0" "EB19CCB1" "A313D55C" "DA56C9EC" "2EF29632"
	"387FE8D7" "6E3C0468" "043E8F66" "3F4860EE" "12BF2D5B" "0B7474D6" "E694F91E"
	"6DCC4024" "FFFFFFFF" "FFFFFFFF";
	return (dh_new_group_asc(gen, group17));
}

DH *dh_new_group18(void)
{
    static char *gen = "2", *group18 =
	"FFFFFFFF" "FFFFFFFF" "C90FDAA2" "2168C234" "C4C6628B" "80DC1CD1"
	"29024E08" "8A67CC74" "020BBEA6" "3B139B22" "514A0879" "8E3404DD"
	"EF9519B3" "CD3A431B" "302B0A6D" "F25F1437" "4FE1356D" "6D51C245"
	"E485B576" "625E7EC6" "F44C42E9" "A637ED6B" "0BFF5CB6" "F406B7ED"
	"EE386BFB" "5A899FA5" "AE9F2411" "7C4B1FE6" "49286651" "ECE45B3D"
	"C2007CB8" "A163BF05" "98DA4836" "1C55D39A" "69163FA8" "FD24CF5F"
	"83655D23" "DCA3AD96" "1C62F356" "208552BB" "9ED52907" "7096966D"
	"670C354E" "4ABC9804" "F1746C08" "CA18217C" "32905E46" "2E36CE3B"
	"E39E772C" "180E8603" "9B2783A2" "EC07A28F" "B5C55DF0" "6F4C52C9"
	"DE2BCBF6" "95581718" "3995497C" "EA956AE5" "15D22618" "98FA0510"
	"15728E5A" "8AAAC42D" "AD33170D" "04507A33" "A85521AB" "DF1CBA64"
	"ECFB8504" "58DBEF0A" "8AEA7157" "5D060C7D" "B3970F85" "A6E1E4C7"
	"ABF5AE8C" "DB0933D7" "1E8C94E0" "4A25619D" "CEE3D226" "1AD2EE6B"
	"F12FFA06" "D98A0864" "D8760273" "3EC86A64" "521F2B18" "177B200C"
	"BBE11757" "7A615D6C" "770988C0" "BAD946E2" "08E24FA0" "74E5AB31"
	"43DB5BFC" "E0FD108E" "4B82D120" "A9210801" "1A723C12" "A787E6D7"
	"88719A10" "BDBA5B26" "99C32718" "6AF4E23C" "1A946834" "B6150BDA"
	"2583E9CA" "2AD44CE8" "DBBBC2DB" "04DE8EF9" "2E8EFC14" "1FBECAA6"
	"287C5947" "4E6BC05D" "99B2964F" "A090C3A2" "233BA186" "515BE7ED"
	"1F612970" "CEE2D7AF" "B81BDD76" "2170481C" "D0069127" "D5B05AA9"
	"93B4EA98" "8D8FDDC1" "86FFB7DC" "90A6C08F" "4DF435C9" "34028492"
	"36C3FAB4" "D27C7026" "C1D4DCB2" "602646DE" "C9751E76" "3DBA37BD"
	"F8FF9406" "AD9E530E" "E5DB382F" "413001AE" "B06A53ED" "9027D831"
	"179727B0" "865A8918" "DA3EDBEB" "CF9B14ED" "44CE6CBA" "CED4BB1B"
	"DB7F1447" "E6CC254B" "33205151" "2BD7AF42" "6FB8F401" "378CD2BF"
	"5983CA01" "C64B92EC" "F032EA15" "D1721D03" "F482D7CE" "6E74FEF6"
	"D55E702F" "46980C82" "B5A84031" "900B1C9E" "59E7C97F" "BEC7E8F3"
	"23A97A7E" "36CC88BE" "0F1D45B7" "FF585AC5" "4BD407B2" "2B4154AA"
	"CC8F6D7E" "BF48E1D8" "14CC5ED2" "0F8037E0" "A79715EE" "F29BE328"
	"06A1D58B" "B7C5DA76" "F550AA3D" "8A1FBFF0" "EB19CCB1" "A313D55C"
	"DA56C9EC" "2EF29632" "387FE8D7" "6E3C0468" "043E8F66" "3F4860EE"
	"12BF2D5B" "0B7474D6" "E694F91E" "6DBE1159" "74A3926F" "12FEE5E4"
	"38777CB6" "A932DF8C" "D8BEC4D0" "73B931BA" "3BC832B6" "8D9DD300"
	"741FA7BF" "8AFC47ED" "2576F693" "6BA42466" "3AAB639C" "5AE4F568"
	"3423B474" "2BF1C978" "238F16CB" "E39D652D" "E3FDB8BE" "FC848AD9"
	"22222E04" "A4037C07" "13EB57A8" "1A23F0C7" "3473FC64" "6CEA306B"
	"4BCBC886" "2F8385DD" "FA9D4B7F" "A2C087E8" "79683303" "ED5BDD3A"
	"062B3CF5" "B3A278A6" "6D2A13F8" "3F44F82D" "DF310EE0" "74AB6A36"
	"4597E899" "A0255DC1" "64F31CC5" "0846851D" "F9AB4819" "5DED7EA1"
	"B1D510BD" "7EE74D73" "FAF36BC3" "1ECFA268" "359046F4" "EB879F92"
	"4009438B" "481C6CD7" "889A002E" "D5EE382B" "C9190DA6" "FC026E47"
	"9558E447" "5677E9AA" "9E3050E2" "765694DF" "C81F56E8" "80B96E71"
	"60C980DD" "98EDD3DF" "FFFFFFFF" "FFFFFFFF";
	return (dh_new_group_asc(gen, group18));
}


// DH���𐶐�����
void dh_gen_key(PTInstVar pvar, DH *dh, int we_need /* bytes */ )
{
	int i;
	BIGNUM *pub_key;
	BIGNUM *priv_key;

	priv_key = NULL;

	// �閧�ɂ��ׂ�����(X)�𐶐�
	for (i = 0 ; i < 10 ; i++) { // retry counter
		if (priv_key != NULL) {
			BN_clear_free(priv_key);
		}
		priv_key = BN_new();
		DH_set0_key(dh, NULL, priv_key);
		if (priv_key == NULL)
			goto error;
		if (BN_rand(priv_key, 2*(we_need*8), 0, 0) == 0)
			goto error;
		if (DH_generate_key(dh) == 0)
			goto error;
		DH_get0_key(dh, &pub_key, NULL);
		if (dh_pub_is_valid(dh, pub_key))
			break;
	}
	if (i >= 10) {
		goto error;
	}
	return;

error:;
	notify_fatal_error(pvar, "error occurred @ dh_gen_key()", TRUE);

}


int dh_estimate(int bits)
{
	if (bits <= 112)
		return 2048;
	if (bits <= 128)
		return 3072;
	if (bits <= 192)
		return 7680;
	return 8192;
}


// shared secret ���v�Z���� (DH �Œ�O���[�v�p)
unsigned char *kex_dh_hash(const EVP_MD *evp_md,
                           char *client_version_string,
                           char *server_version_string,
                           char *ckexinit, int ckexinitlen,
                           char *skexinit, int skexinitlen,
                           u_char *serverhostkeyblob, int sbloblen,
                           BIGNUM *client_dh_pub,
                           BIGNUM *server_dh_pub,
                           BIGNUM *shared_secret,
                           unsigned int *hashlen)
{
	buffer_t *b;
	static unsigned char digest[EVP_MAX_MD_SIZE];
	EVP_MD_CTX *md = NULL;

	md = EVP_MD_CTX_new();
	if (md == NULL)
		goto error;

	b = buffer_init();
	buffer_put_string(b, client_version_string, strlen(client_version_string));
	buffer_put_string(b, server_version_string, strlen(server_version_string));

	/* kexinit messages: fake header: len+SSH2_MSG_KEXINIT */
	buffer_put_int(b, ckexinitlen+1);
	buffer_put_char(b, SSH2_MSG_KEXINIT);
	buffer_append(b, ckexinit, ckexinitlen);
	buffer_put_int(b, skexinitlen+1);
	buffer_put_char(b, SSH2_MSG_KEXINIT);
	buffer_append(b, skexinit, skexinitlen);

	buffer_put_string(b, serverhostkeyblob, sbloblen);
	buffer_put_bignum2(b, client_dh_pub);
	buffer_put_bignum2(b, server_dh_pub);
	buffer_put_bignum2(b, shared_secret);

	// yutaka
	//debug_print(38, buffer_ptr(b), buffer_len(b));

	EVP_DigestInit(md, evp_md);
	EVP_DigestUpdate(md, buffer_ptr(b), buffer_len(b));
	EVP_DigestFinal(md, digest, NULL);

	buffer_free(b);

	//write_buffer_file(digest, EVP_MD_size(evp_md));

	*hashlen = EVP_MD_size(evp_md);

error:
	if (md)
		EVP_MD_CTX_free(md);

	return digest;
}


// shared secret ���v�Z���� (DH GEX�p)
unsigned char *kex_dh_gex_hash(const EVP_MD *evp_md,
                               char *client_version_string,
                               char *server_version_string,
                               char *ckexinit, int ckexinitlen,
                               char *skexinit, int skexinitlen,
                               u_char *serverhostkeyblob, int sbloblen,
                               int kexgex_min,
                               int kexgex_bits,
                               int kexgex_max,
                               BIGNUM *kexgex_p,
                               BIGNUM *kexgex_g,
                               BIGNUM *client_dh_pub,
                               BIGNUM *server_dh_pub,
                               BIGNUM *shared_secret,
                               unsigned int *hashlen)
{
	buffer_t *b;
	static unsigned char digest[EVP_MAX_MD_SIZE];
	EVP_MD_CTX *md = NULL;

	md = EVP_MD_CTX_new();
	if (md == NULL)
		goto error;

	b = buffer_init();
	buffer_put_string(b, client_version_string, strlen(client_version_string));
	buffer_put_string(b, server_version_string, strlen(server_version_string));

	/* kexinit messages: fake header: len+SSH2_MSG_KEXINIT */
	buffer_put_int(b, ckexinitlen+1);
	buffer_put_char(b, SSH2_MSG_KEXINIT);
	buffer_append(b, ckexinit, ckexinitlen);
	buffer_put_int(b, skexinitlen+1);
	buffer_put_char(b, SSH2_MSG_KEXINIT);
	buffer_append(b, skexinit, skexinitlen);

	buffer_put_string(b, serverhostkeyblob, sbloblen);

	// DH group size�̃r�b�g�������Z����
	buffer_put_int(b, kexgex_min);
	buffer_put_int(b, kexgex_bits);
	buffer_put_int(b, kexgex_max);

	// DH���̑f���Ɛ����������Z����
	buffer_put_bignum2(b, kexgex_p);
	buffer_put_bignum2(b, kexgex_g);

	buffer_put_bignum2(b, client_dh_pub);
	buffer_put_bignum2(b, server_dh_pub);
	buffer_put_bignum2(b, shared_secret);

	// yutaka
	//debug_print(38, buffer_ptr(b), buffer_len(b));

	EVP_DigestInit(md, evp_md);
	EVP_DigestUpdate(md, buffer_ptr(b), buffer_len(b));
	EVP_DigestFinal(md, digest, NULL);

	buffer_free(b);

	//write_buffer_file(digest, EVP_MD_size(evp_md));

	*hashlen = EVP_MD_size(evp_md);

error:
	if (md)
		EVP_MD_CTX_free(md);

	return digest;
}


unsigned char *kex_ecdh_hash(const EVP_MD *evp_md,
                             const EC_GROUP *ec_group,
                             char *client_version_string,
                             char *server_version_string,
                             char *ckexinit, int ckexinitlen,
                             char *skexinit, int skexinitlen,
                             u_char *serverhostkeyblob, int sbloblen,
                             const EC_POINT *client_dh_pub,
                             const EC_POINT *server_dh_pub,
                             BIGNUM *shared_secret,
                             unsigned int *hashlen)
{
	buffer_t *b;
	static unsigned char digest[EVP_MAX_MD_SIZE];
	EVP_MD_CTX *md = NULL;

	md = EVP_MD_CTX_new();
	if (md == NULL)
		goto error;

	b = buffer_init();
	buffer_put_string(b, client_version_string, strlen(client_version_string));
	buffer_put_string(b, server_version_string, strlen(server_version_string));

	/* kexinit messages: fake header: len+SSH2_MSG_KEXINIT */
	buffer_put_int(b, ckexinitlen+1);
	buffer_put_char(b, SSH2_MSG_KEXINIT);
	buffer_append(b, ckexinit, ckexinitlen);
	buffer_put_int(b, skexinitlen+1);
	buffer_put_char(b, SSH2_MSG_KEXINIT);
	buffer_append(b, skexinit, skexinitlen);

	buffer_put_string(b, serverhostkeyblob, sbloblen);

	buffer_put_ecpoint(b, ec_group, client_dh_pub);
	buffer_put_ecpoint(b, ec_group, server_dh_pub);
	buffer_put_bignum2(b, shared_secret);

	// yutaka
	//debug_print(38, buffer_ptr(b), buffer_len(b));

	EVP_DigestInit(md, evp_md);
	EVP_DigestUpdate(md, buffer_ptr(b), buffer_len(b));
	EVP_DigestFinal(md, digest, NULL);

	buffer_free(b);

	//write_buffer_file(digest, EVP_MD_size(evp_md));

	*hashlen = EVP_MD_size(evp_md);

error:
	if (md)
		EVP_MD_CTX_free(md);

	return digest;
}


int dh_pub_is_valid(DH *dh, BIGNUM *dh_pub)
{
	int i;
	int n = BN_num_bits(dh_pub);
	int bits_set = 0;
	const BIGNUM *p;

	// OpenSSL 1.1.0�ŁABIGNUM�\���̂�neg�����o�[�ɒ��ڃA�N�Z�X�ł��Ȃ��Ȃ������߁A
	// BN_is_negative�֐��ɒu������BOpenSSL 1.0.2�ł̓}�N����`����Ă���̂ŁA
	// OpenSSL 1.0.2�ł��A���̏������ł悢�B
	if (BN_is_negative(dh_pub)) {
		//logit("invalid public DH value: negativ");
		return 0;
	}
	for (i = 0; i <= n; i++)
		if (BN_is_bit_set(dh_pub, i))
			bits_set++;
	//debug2("bits set: %d/%d", bits_set, BN_num_bits(dh->p));

	/* if g==2 and bits_set==1 then computing log_g(dh_pub) is trivial */
	DH_get0_pqg(dh, &p, NULL, NULL);
	if (bits_set > 1 && (BN_cmp(dh_pub, p) == -1))
		return 1;
	//logit("invalid public DH value (%d/%d)", bits_set, BN_num_bits(dh->p));
	return 0;
}


static u_char *derive_key(int id, int need, u_char *hash, BIGNUM *shared_secret,
                          char *session_id, int session_id_len,
                          const EVP_MD *evp_md)
{
	buffer_t *b;
	EVP_MD_CTX *md = NULL;
	char c = id;
	int have;
	int mdsz = EVP_MD_size(evp_md);
	u_char *digest = malloc(roundup(need, mdsz));

	md = EVP_MD_CTX_new();
	if (md == NULL)
		goto skip;

	if (digest == NULL)
		goto skip;

	b = buffer_init();
	if (b == NULL)
		goto skip;

	buffer_put_bignum2(b, shared_secret);

	/* K1 = HASH(K || H || "A" || session_id) */
	EVP_DigestInit(md, evp_md);
	EVP_DigestUpdate(md, buffer_ptr(b), buffer_len(b));
	EVP_DigestUpdate(md, hash, mdsz);
	EVP_DigestUpdate(md, &c, 1);
	EVP_DigestUpdate(md, session_id, session_id_len);
	EVP_DigestFinal(md, digest, NULL);

	/*
	 * expand key:
	 * Kn = HASH(K || H || K1 || K2 || ... || Kn-1)
	 * Key = K1 || K2 || ... || Kn
	 */
	for (have = mdsz; need > have; have += mdsz) {
		EVP_DigestInit(md, evp_md);
		EVP_DigestUpdate(md, buffer_ptr(b), buffer_len(b));
		EVP_DigestUpdate(md, hash, mdsz);
		EVP_DigestUpdate(md, digest, have);
		EVP_DigestFinal(md, digest + have, NULL);
	}
	buffer_free(b);

skip:;
	if (md)
		EVP_MD_CTX_free(md);

	return digest;
}

/*
 * �������̌��ʂ���e���𐶐��� newkeys �ɃZ�b�g���Ė߂��B
 */
void kex_derive_keys(PTInstVar pvar, SSHKeys *newkeys, int need, u_char *hash, BIGNUM *shared_secret,
                     char *session_id, int session_id_len)
{
#define NKEYS	6
	u_char *keys[NKEYS];
	int i, mode, ctos;

	for (i = 0; i < NKEYS; i++) {
		keys[i] = derive_key('A'+i, need, hash, shared_secret, session_id, session_id_len,
		                     get_kex_algorithm_EVP_MD(pvar->kex_type));
		//debug_print(i, keys[i], need);
	}

	for (mode = 0; mode < MODE_MAX; mode++) {
		if (mode == MODE_OUT)
			ctos = 1;
		else
			ctos = 0;

		// setting
		newkeys[mode].enc.iv  = keys[ctos ? 0 : 1];
		newkeys[mode].enc.key = keys[ctos ? 2 : 3];
		newkeys[mode].mac.key = keys[ctos ? 4 : 5];

		//debug_print(20 + mode*3, newkeys[mode]->enc.iv, 8);
		//debug_print(21 + mode*3, newkeys[mode]->enc.key, 24);
		//debug_print(22 + mode*3, newkeys[mode]->mac.key, 24);
	}
}
