/******************************************************************************
 * Copyright  2014-2019 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#include <stdint.h>
#include <string.h>
#include <numeric>
#include "univalue.h"
#include "amount.h"
#include "rpc/server.h"
#include "rpc/protocol.h"

#include "../wallet/crypter.h"
#include "../wallet/rpcwallet.h"

#include "sync_ext.h"

#include "../cc/CCinclude.h"
#include "../cc/CCtokens.h"
#include "../cc/CCassets.h"

#include "../cc/CCtokens_impl.h"
#include "../cc/CCassetstx_impl.h"

using namespace std;

UniValue assetsaddress(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
	struct CCcontract_info *cp, C; std::vector<unsigned char> pubkey;
	cp = CCinit(&C, EVAL_ASSETS);
	if (fHelp || params.size() > 1)
		throw runtime_error("assetsaddress [pubkey]\n");
	if (ensure_CCrequirements(cp->evalcode) < 0)
		throw runtime_error(CC_REQUIREMENTS_MSG);
	if (params.size() == 1)
		pubkey = ParseHex(params[0].get_str().c_str());
	return CCaddress(cp, (char *)"Assets", pubkey);
}

UniValue tokenaddress(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    struct CCcontract_info *cp,C; std::vector<unsigned char> pubkey;
    cp = CCinit(&C, EVAL_TOKENS);
    if ( fHelp || params.size() > 1 )
        throw runtime_error("tokenaddress [pubkey]\n");
    if ( ensure_CCrequirements(cp->evalcode) < 0 )
        throw runtime_error(CC_REQUIREMENTS_MSG);
    if ( params.size() == 1 )
        pubkey = ParseHex(params[0].get_str().c_str());
    return CCaddress(cp, "Tokens", pubkey, false);
}

UniValue tokenv2address(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    struct CCcontract_info *cp,C; 
    vuint8_t pubkey;

    cp = CCinit(&C, EVAL_TOKENSV2);
    if (fHelp || params.size() > 1)
        throw runtime_error("tokenv2address [pubkey]\n");
    if (ensure_CCrequirements(cp->evalcode) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    if (params.size() == 1)
        pubkey = ParseHex(params[0].get_str().c_str());
    return CCaddress(cp, "Tokensv2", pubkey, true);  
}

UniValue assetsv2address(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    struct CCcontract_info *cp,C; 
    vuint8_t pubkey;

    cp = CCinit(&C, EVAL_ASSETSV2);
    if (fHelp || params.size() > 1)
        throw runtime_error("assetsv2address [pubkey]\n");
    if (ensure_CCrequirements(cp->evalcode) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    if (params.size() == 1)
        pubkey = ParseHex(params[0].get_str().c_str());
    return CCaddress(cp, "Assetsv2", pubkey, true);  
}

UniValue tokenlist(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    uint256 tokenid;
    if (fHelp || params.size() > 0)
        throw runtime_error("tokenlist\n");
        
    if (ensure_CCrequirements(EVAL_TOKENS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);

    return TokenList();
}
UniValue tokenv2list(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    const static std::set<std::string> acceptable = { "beginHeight", "endHeight", "pubkey", "address" };

    if (fHelp || params.size() > 1)
        throw runtime_error("tokenv2list [json-params]\n"
                            "json-params optional params as a json object, limiting tokenv2list output:\n"
                            "  { \"beginHeight\": number \"endHeight\": number, \"pubkey\": hexstring, \"address\": string }\n"
                            "  \"beginHeight\", \"endHeight\" - height interval where to search tokenv2create transactions, if beginHeight omitted the first block used, if endHeight omitted the chain tip used"
                            "  \"pubkey\" - search tokens created by a specific pubkey\n"
                            "  \"address\" - search created on a specific cc address\n");

    if (ensure_CCrequirements(EVAL_TOKENSV2) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);

    UniValue jsonParams;
    if (params.size() == 1)
    {
        if (params[0].getType() == UniValue::VOBJ)
            jsonParams = params[0].get_array();
        else if (params[0].getType() == UniValue::VSTR)  // json in quoted string '{...}'
            jsonParams.read(params[0].get_str().c_str());
        if (jsonParams.getType() != UniValue::VOBJ)
            throw runtime_error("parameter 1 must be a json object");   

        // check unused params:
        for (int i = 0; i < jsonParams.getKeys().size(); i ++)
            if (acceptable.count(jsonParams.getKeys()[i]) == 0)
                throw runtime_error(std::string("invalid json param") + jsonParams.getKeys()[i]);   
    }
    return TokenV2List(jsonParams);
}

template <class V>
static UniValue tokeninfo(const std::string& name, const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(name + " tokenid\n");
    if (ensure_CCrequirements(V::EvalCode()) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    uint256 tokenid = Parseuint256((char *)params[0].get_str().c_str());
    return TokenInfo<V>(tokenid, [](const vuint8_t &data){ return NullUniValue; });
}

UniValue tokeninfo(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokeninfo<TokensV1>("tokeninfo", params, fHelp, remotepk);
}
UniValue tokenv2info(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokeninfo<TokensV2>("tokenv2info", params, fHelp, remotepk);
}

template <class V>
static UniValue tokeninfotokel(const std::string& name, const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(name + " tokenid\n");
    if (ensure_CCrequirements(V::EvalCode()) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    uint256 tokenid = Parseuint256((char *)params[0].get_str().c_str());
    return TokenInfo<V>(tokenid, ParseTokelVData);
}

UniValue tokeninfotokel(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokeninfotokel<TokensV1>("tokeninfotokel", params, fHelp, remotepk);
}
UniValue tokenv2infotokel(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokeninfotokel<TokensV2>("tokenv2infotokel", params, fHelp, remotepk);
}

template <class T, class A>
UniValue tokenorders(const std::string& name, const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    uint256 tokenid;
    const CPubKey emptypk;

    if (fHelp || params.size() > 1)
        throw runtime_error(name + " [tokenid|'*']\n"
                                   "returns tokens orders for the tokenid or all available token orders if tokenid is not set\n"
                                   "\n");
    if (ensure_CCrequirements(A::EvalCode()) < 0 || ensure_CCrequirements(T::EvalCode()) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
	if (params.size() >= 1) 
    {
        if (params[0].get_str() != "*")
        {
		    tokenid = Parseuint256((char *)params[0].get_str().c_str());
		    if (tokenid == zeroid) 
			    throw runtime_error("incorrect tokenid\n");
        }
    }
    return AssetOrders<T, A>(tokenid, emptypk);
}

UniValue tokenorders(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokenorders<TokensV1, AssetsV1>("tokenorders", params, fHelp, remotepk);
}
UniValue tokenv2orders(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokenorders<TokensV2, AssetsV2>("tokenv2orders", params, fHelp, remotepk);
}

template <class T, class A>
UniValue mytokenorders(const std::string& name, const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    if (fHelp || params.size() > 0)
        throw runtime_error(name + "\n"
                            "returns all tokens orders for mypubkey\n"
                            // no additional evalcode for mytokenorders - it will return all orders for on mypk:
                            /*"if evalcode is set then returns mypubkey's token orders for non-fungible tokens with this evalcode\n"*/ "\n");
    if (ensure_CCrequirements(A::EvalCode()) < 0 || ensure_CCrequirements(T::EvalCode()) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);


    CPubKey mypk;
    SET_MYPK_OR_REMOTE(mypk, remotepk);
    return AssetOrders<T, A>(zeroid, mypk);
}

UniValue mytokenorders(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return mytokenorders<TokensV1, AssetsV1>("mytokenorders", params, fHelp, remotepk);
}
UniValue mytokenv2orders(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return mytokenorders<TokensV2, AssetsV2>("mytokenv2orders", params, fHelp, remotepk);
}

template <class V>
static UniValue tokenbalance(const std::string& name, const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    UniValue result(UniValue::VOBJ);
    CCerror.clear();

    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(name + " tokenid [pubkey]\n");
    if (ensure_CCrequirements(V::EvalCode()) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);

    // LOCK(cs_main); 
    // no need to lock cs_main as we use only indexes in this rpc
    // but still use lock if you need to get chainActive.Height() or something like that

    uint256 tokenid = Parseuint256((char *)params[0].get_str().c_str());
    std::vector<unsigned char> vpubkey;
    if (params.size() == 2)
        vpubkey = ParseHex(params[1].get_str().c_str());
    else
        vpubkey = Mypubkey();

    CAmount balance = GetTokenBalance<V>(pubkey2pk(vpubkey), tokenid, false);

	if (CCerror.empty()) {
		char destaddr[KOMODO_ADDRESS_BUFSIZE];
        struct CCcontract_info *cp, C;
        cp = CCinit(&C, V::EvalCode());

		result.push_back(Pair("result", "success"));
		if (GetCCaddress(cp, destaddr, pubkey2pk(vpubkey), V::IsMixed()) != 0)
			result.push_back(Pair("CCaddress", destaddr));

		result.push_back(Pair("tokenid", params[0].get_str()));
		result.push_back(Pair("balance", (int64_t)balance));
	}
	else {
		result = MakeResultError(CCerror);
	}

    return result;
}

UniValue tokenbalance(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokenbalance<TokensV1>("tokenbalance", params, fHelp, remotepk);
}
UniValue tokenv2balance(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokenbalance<TokensV2>("tokenv2balance", params, fHelp, remotepk);
}

template <class V>
static UniValue tokencreate(const UniValue& params, const vuint8_t &vtokenData, bool fHelp, const CPubKey& remotepk)
{
    UniValue result(UniValue::VOBJ);
    std::string name, description, hextx; 
    CAmount supply; // changed from uin64_t to int64_t for this 'if ( supply <= 0 )' to work as expected

    CCerror.clear();

    //if (fHelp || params.size() > 4 || params.size() < 2)
    //    throw runtime_error(fname + " name supply [description] [tokens data]\n");
    if (ensure_CCrequirements(V::EvalCode()) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);

    if (!remotepk.IsValid() && !EnsureWalletIsAvailable(false))
        throw runtime_error("wallet is required");
    LOCK2(cs_main, pwalletMain->cs_wallet);  // remote call not supported yet

    name = params[0].get_str();
    if (name.size() == 0 || name.size() > TOKENS_MAX_NAME_LENGTH)   
        return MakeResultError("Token name must not be empty and up to " + std::to_string(TOKENS_MAX_DESC_LENGTH));

    supply = AmountFromValue(params[1]);   
    if (supply <= 0)    
        return MakeResultError("Token supply must be positive");
    
    if (params.size() >= 3)     {
        description = params[2].get_str();
        if (description.size() > TOKENS_MAX_DESC_LENGTH)   
            return MakeResultError("Token description must be <= " + std::to_string(TOKENS_MAX_DESC_LENGTH));  // allowed > MAX_SCRIPT_ELEMENT_SIZE
    }

    hextx = CreateTokenLocal<V>(0, supply, name, description, vtokenData);
    RETURN_IF_ERROR(CCerror);

    if( hextx.size() > 0 )     
        return MakeResultSuccess(hextx);
    else
        return MakeResultError("could not create token");
}

UniValue tokencreate(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    if (fHelp || params.size() > 4 || params.size() < 2)
        throw runtime_error("tokencreate name supply [description] [tokens data]\n"
                            "create tokens\n"
                            "  name - token name string\n"
                            "  supply - token supply in coins\n"
                            "  description - optional description"
                            "  tokens data - an optional hex string with token data. If the first byte is non-null it means a evalcode which these tokens will be routed into\n"
        );

    vuint8_t tokenData;
    if (params.size() >= 4)    {
        tokenData = ParseHex(params[3].get_str());
        if (tokenData.empty())
            return MakeResultError("Token data incorrect");
    }
    return tokencreate<TokensV1>(params, tokenData, fHelp, remotepk);
}
UniValue tokenv2create(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
if (fHelp || params.size() > 4 || params.size() < 2)
        throw runtime_error("tokenv2create name supply [description] [tokens data]\n"
                            "create tokens version 2\n"
                            "  name - token name string\n"
                            "  supply - token supply in coins\n"
                            "  description - optional description"
                            "  tokens data - an optional hex string with token data. If the first byte is non-null it means a evalcode which these tokens will be routed into\n"
        );

    vuint8_t tokenData;
    if (params.size() >= 4)    {
        tokenData = ParseHex(params[3].get_str());
        if (tokenData.empty())
            return MakeResultError("Tokel token data incorrect");
    }
    return tokencreate<TokensV2>(params, tokenData, fHelp, remotepk);
}

UniValue tokencreatetokel(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    if (fHelp || params.size() > 4 || params.size() < 2)
        throw runtime_error("tokencreatetokel name supply [description] [tokens data]\n"
                            "create tokens with tokel project data\n"
                            "  name - token name string\n"
                            "  supply - token supply in coins\n"
                            "  description - opt description"
                            "  tokens data - an optional json object with tokel token properties:\n"
                            "     { \"url\":<url-string>, \"id\":<token application id>, \"royalty\":<royalty 0..999>, \"arbitrary\":<arbitrary-data-hex> }\n"
        );

    vuint8_t tokenData;
    if (params.size() >= 4)    {
        UniValue jsonParams;
        std::string sError;

        if (params[3].getType() == UniValue::VOBJ)
            jsonParams = params[3].get_array();
        else if (params[3].getType() == UniValue::VSTR)  // json in quoted string '{...}'
            jsonParams.read(params[3].get_str().c_str());
        if (jsonParams.getType() != UniValue::VOBJ)
            throw runtime_error("parameter 4 must be a json object\n");   

        tokenData = ParseTokelJson(jsonParams);
        if (tokenData.empty())
            throw runtime_error("Tokel token data incorrect");
        if (!CheckTokelData(tokenData, sError))
            throw runtime_error("Tokel token data incorrect: " + sError);
    }

    return tokencreate<TokensV1>(params, tokenData, fHelp, remotepk);
}
UniValue tokenv2createtokel(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    if (fHelp || params.size() > 4 || params.size() < 2)
        throw runtime_error("tokenv2createtokel name supply [description] [tokens data]\n"
                            "create tokens with tokel project data\n"
                            "  name - token name string\n"
                            "  supply - token supply in coins\n"
                            "  description - opt description"
                            "  tokens data - an opt json object with with tokel token properties:\n"
                            "     { \"url\":<url-string>, \"id\":<token application id>, \"royalty\":<royalty 0..999>, \"arbitrary\":<arbitrary-data-hex> }\n"
        );

    vuint8_t tokenData;
    if (params.size() >= 4)    {
        UniValue jsonParams;
        std::string sError;

        if (params[3].getType() == UniValue::VOBJ)
            jsonParams = params[3].get_array();
        else if (params[3].getType() == UniValue::VSTR)  // json in quoted string '{...}'
            jsonParams.read(params[3].get_str().c_str());
        if (jsonParams.getType() != UniValue::VOBJ)
            return MakeResultError("parameter 4 must be a json object\n");   

        tokenData = ParseTokelJson(jsonParams);
        if (tokenData.empty())
            return MakeResultError("Token data incorrect");
        if (!CheckTokelData(tokenData, sError))
            throw runtime_error("Tokel token data incorrect: " + sError);
    }
    return tokencreate<TokensV2>(params, tokenData, fHelp, remotepk);
}


template <class V>
static UniValue tokentransfer(const std::string& name, const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    UniValue result(UniValue::VOBJ); 
    std::string hex; 
    
    CCerror.clear();

    if (fHelp || (params.size() != 3 && params.size() != 1))
        throw runtime_error(name + " tokenid destpubkey amount\n" +
                            name + " '{ \"tokenid\":\"<tokenid>\", \"ccaddressMofN\":\"<address>\", \"destpubkeys\": [ \"<pk1>\", \"<pk2>\", ... ], \"M\": <value>, \"amount\": <amount> }'\n"
                            "tokenid - token creation id\n"
                            "ccaddressMofN - optional cc address of MofN utxos to spend, if not present spending is from mypk\n"
                            "destpubkey, destpubkey1 ... destpubkeyN - destination pubkeys (max = 128)\n"
                            "M - required min of signatures, integer\n\n"
                            "amount - token amount to send in satoshi, int64\n"
                            "Note, that MofN supported only for tokens v2\n\n");

    if (ensure_CCrequirements(V::EvalCode()) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    
    if (!remotepk.IsValid() && !EnsureWalletIsAvailable(false))
        throw runtime_error("wallet is required");    
    LOCK2(cs_main, pwalletMain->cs_wallet);   // remote call not supported yet
    
    if (params.size() == 3)
    {
        uint256 tokenid = Parseuint256((char *)params[0].get_str().c_str());
        if( tokenid == zeroid )    
            return MakeResultError("invalid tokenid");
        
        std::vector<CPubKey> pks;
        vuint8_t vpubkey(ParseHex(params[1].get_str().c_str()));
        if (vpubkey.size() != CPubKey::COMPRESSED_PUBLIC_KEY_SIZE) 
            return MakeResultError("invalid destpubkey");    
        pks.push_back(pubkey2pk(vpubkey));

        CAmount amount = atoll(params[2].get_str().c_str()); 
        if( amount <= 0 )    
            return MakeResultError("amount must be positive");
        hex = TokenTransfer<V>(0, tokenid, 1, pks, amount);
        RETURN_IF_ERROR(CCerror);
        if (!hex.empty())
            return MakeResultSuccess(hex);
        else
            return MakeResultError("could not create transfer token transaction");
    }
    else
    {
        if (V::EvalCode() != EVAL_TOKENSV2)
            return MakeResultError("MofN transfer is supported only for tokens v2\n");

        UniValue jsonParams(UniValue::VOBJ);
        if (params[0].getType() == UniValue::VOBJ)
            jsonParams = params[0].get_obj();
        else if (params[0].getType() == UniValue::VSTR)  // json in quoted string '{...}'
            jsonParams.read(params[0].get_str().c_str());
        if (jsonParams.getType() != UniValue::VOBJ)
            return MakeResultError("parameter 1 must be object\n");

        uint256 tokenid = Parseuint256(jsonParams["tokenid"].get_str().c_str());
        if( tokenid == zeroid )    
            return MakeResultError("invalid tokenid");
        
        std::string ccaddressMofN;
        if (!jsonParams["ccaddressMofN"].isNull()) {
            ccaddressMofN = jsonParams["ccaddressMofN"].get_str();
            if (!CBitcoinAddress(ccaddressMofN).IsValid())        
                return MakeResultError("invalid ccaddressMofN\n");
        }

        std::vector<CPubKey> pks;
        UniValue udestpks = jsonParams["destpubkeys"];
        if (!udestpks.isArray())
            return MakeResultError("destpubkeys must be an array\n");
        
        if (udestpks.size() > 128)
            return MakeResultError("destpubkeys num is limited by 128\n");

        for (int i = 0; i < udestpks.size(); i ++) {
            vuint8_t vpubkey(ParseHex(udestpks[i].get_str().c_str()));
            if (vpubkey.size() != CPubKey::COMPRESSED_PUBLIC_KEY_SIZE) 
                return MakeResultError(std::string("invalid destpubkey #") + std::to_string(i+1));
            pks.push_back(CPubKey(vpubkey));
        }
        int M = jsonParams["M"].get_int();
        if (M > 128)
            return MakeResultError("M is limited by 128\n");
        if (M > pks.size())
            return MakeResultError("M could not be more than dest pubkeys\n");

        CAmount amount = jsonParams["amount"].get_int64(); 
        if( amount <= 0 )    
            return MakeResultError("amount must be positive");

        if (ccaddressMofN.empty()) {
            hex = TokenTransfer<V>(0, tokenid, M, pks, amount);
            RETURN_IF_ERROR(CCerror);
            if (!hex.empty())
                return MakeResultSuccess(hex);
            else
                return MakeResultError("could not create transfer token transaction");
        }
        else {
            UniValue transferred = TokenTransferExt<V>(CPubKey(), 0, tokenid, ccaddressMofN.c_str(), {}, (uint8_t)M, pks, amount, false);
            RETURN_IF_ERROR(CCerror);
            if (!ResultGetTx(transferred).empty())
                return transferred;
            else
                return MakeResultError("could not create transfer token transaction");
        }     
    }
}

UniValue tokentransfer(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokentransfer<TokensV1>("tokentransfer", params, fHelp, remotepk);
}
UniValue tokenv2transfer(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokentransfer<TokensV2>("tokenv2transfer", params, fHelp, remotepk);
}

template <class V>
UniValue tokentransfermany(const std::string& name, const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    UniValue result(UniValue::VOBJ); 
    std::string hex; 
    CAmount amount; 
    uint256 tokenid;
    const CAmount txfee = 10000;
    
    CCerror.clear();

    if (fHelp || params.size() < 3)
        throw runtime_error(name + " tokenid1 tokenid2 ... destpubkey amount \n");
    if (ensure_CCrequirements(V::EvalCode()) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);

    std::vector<uint256> tokenids;
    int i = 0;
    for (; i < params.size() - 2; i ++)
    {
        uint256 tokenid = Parseuint256((char *)params[i].get_str().c_str());
        if( tokenid == zeroid )    
            return MakeResultError("invalid tokenid");
        tokenids.push_back(tokenid);
    }
    CPubKey destpk = pubkey2pk(ParseHex(params[i++].get_str().c_str()));
	if (destpk.size() != CPubKey::COMPRESSED_PUBLIC_KEY_SIZE) 
        return MakeResultError("invalid destpubkey");
    
    amount = atoll(params[i].get_str().c_str()); 
    if( amount <= 0 )    
        return MakeResultError("amount must be positive");
    
    if (!remotepk.IsValid() && !EnsureWalletIsAvailable(false))
        throw runtime_error("wallet is required");
    LOCK2(cs_main, pwalletMain->cs_wallet);  // remote call not supported yet

    CPubKey mypk = remotepk.IsValid() ? remotepk : pubkey2pk(Mypubkey());

    CMutableTransaction mtx;
    struct CCcontract_info *cpTokens, CTokens;
    cpTokens = CCinit(&CTokens, V::EvalCode());

    UniValue beginResult = TokenBeginTransferTx<V>(mtx, cpTokens, remotepk, txfee);
    if (ResultIsError(beginResult)) 
        return beginResult;
    
    for (const auto &tokenid : tokenids)
    {
        CCwrapper probeCond;
        probeCond.reset( MakeCCcond1(V::EvalCode(), mypk) );

        uint8_t mypriv[32];
        Myprivkey(mypriv);
        
        char tokenaddr[KOMODO_ADDRESS_BUFSIZE];
        GetTokensCCaddress(cpTokens, tokenaddr, mypk, V::IsMixed());

        UniValue addtxResult = TokenAddTransferVout<V>(mtx, cpTokens, remotepk, tokenid, tokenaddr, { destpk }, {probeCond, mypriv}, amount, false);
        memset(mypriv, '\0', sizeof(mypriv));
        if (ResultIsError(addtxResult)) 
            return MakeResultError( ResultGetError(addtxResult) + " " + tokenid.GetHex() );
    }
    UniValue sigData = TokenFinalizeTransferTx<V>(mtx, cpTokens, remotepk, txfee, CScript());
    RETURN_IF_ERROR(CCerror);
    if (ResultHasTx(sigData) > 0)
        result = sigData;
    else
        result = MakeResultError("could not transfer token: " + ResultGetError(sigData));
    return result;
}

UniValue tokentransfermany(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokentransfermany<TokensV1>("tokentransfermany", params, fHelp, remotepk);
}
UniValue tokenv2transfermany(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokentransfermany<TokensV2>("tokenv2transfermany", params, fHelp, remotepk);
}

UniValue tokenconvert(const UniValue& params, bool fHelp, const CPubKey& mypk)
{
    UniValue result(UniValue::VOBJ); std::string hex; int32_t evalcode; int64_t amount; uint256 tokenid;
    CCerror.clear();
    if (fHelp || params.size() != 4)
        throw runtime_error("tokenconvert evalcode tokenid pubkey amount\n");
    if (ensure_CCrequirements(EVAL_ASSETS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    const CKeyStore& keystore = *pwalletMain;
    if (!EnsureWalletIsAvailable(false))
        throw runtime_error("wallet is required");
    LOCK2(cs_main, pwalletMain->cs_wallet);
    evalcode = atoi(params[0].get_str().c_str());
    tokenid = Parseuint256((char *)params[1].get_str().c_str());
    std::vector<unsigned char> pubkey(ParseHex(params[2].get_str().c_str()));
    //amount = atol(params[3].get_str().c_str());
	amount = atoll(params[3].get_str().c_str()); // dimxy changed to prevent loss of significance
    if( tokenid == zeroid )
        return MakeResultError("invalid tokenid");

    if( amount <= 0 )
        return MakeResultError("amount must be positive");

	return MakeResultError("deprecated");

/*    hex = AssetConvert(0,tokenid,pubkey,amount,evalcode);
    if (amount > 0) {
        if ( hex.size() > 0 )
        {
            result.push_back(Pair("result", "success"));
            result.push_back(Pair("hex", hex));
        } else ERR_RESULT("couldnt convert tokens");
    } else {
        ERR_RESULT("amount must be positive");
    }
    return(result); */
}

template <class T, class A>
UniValue tokenbid(const std::string& name, const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    UniValue result(UniValue::VOBJ); 
    int64_t bidamount,numtokens; 
    std::string hex; 
    uint256 tokenid;

    CCerror.clear();
    if (fHelp || params.size() < 3 || params.size() > 4)
        throw runtime_error(name + " numtokens tokenid price [expiry-height]\n");
    if (ensure_CCrequirements(A::EvalCode()) < 0 || ensure_CCrequirements(T::EvalCode()) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);

    if (!remotepk.IsValid() && !EnsureWalletIsAvailable(false))
        throw runtime_error("wallet is required");
    CONDITIONAL_LOCK2(cs_main, pwalletMain->cs_wallet, !remotepk.IsValid());

	numtokens = atoll(params[0].get_str().c_str());  
    tokenid = Parseuint256((char *)params[1].get_str().c_str());
    CAmount price = AmountFromValue(params[2]);
    bidamount = (price * numtokens);
    if (price <= 0)
        return MakeResultError("price must be positive");
    if (tokenid == zeroid)
        return MakeResultError("invalid tokenid");
    if (bidamount <= 0)
        return MakeResultError("bid amount must be positive");

    int32_t expiryHeight; 
    {
        LOCK(cs_main);
        expiryHeight = chainActive.Height() + 4 * 7 * 24 * 60; // 4 weeks for blocktime 60 sec
    }
    if (params.size() == 4)  {
        expiryHeight = atol(params[3].get_str().c_str());	
        if (!remotepk.IsValid() && expiryHeight < chainActive.LastTip()->GetHeight())
            return MakeResultError("expiry height invalid");

    }

    CPubKey mypk;
    SET_MYPK_OR_REMOTE(mypk, remotepk);
    result = CreateBuyOffer<T, A>(mypk, 0, bidamount, tokenid, numtokens, expiryHeight);
    RETURN_IF_ERROR(CCerror);
    return result;
}

UniValue tokenbid(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokenbid<TokensV1, AssetsV1>("tokenbid", params, fHelp, remotepk);
}
UniValue tokenv2bid(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokenbid<TokensV2, AssetsV2>("tokenv2bid", params, fHelp, remotepk);
}

template <class T, class A>
UniValue tokencancelbid(const std::string& name, const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    UniValue result(UniValue::VOBJ); std::string hex; int32_t i; uint256 tokenid,bidtxid;
    CCerror.clear();
    if (fHelp || params.size() != 2)
        throw runtime_error(name + " tokenid bidtxid\n");

    if (ensure_CCrequirements(A::EvalCode()) < 0 || ensure_CCrequirements(T::EvalCode()) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);

    if (!remotepk.IsValid() && !EnsureWalletIsAvailable(false))
        throw runtime_error("wallet is required");
    CONDITIONAL_LOCK2(cs_main, pwalletMain->cs_wallet, !remotepk.IsValid());

    tokenid = Parseuint256((char *)params[0].get_str().c_str());
    bidtxid = Parseuint256((char *)params[1].get_str().c_str());
    if ( tokenid == zeroid || bidtxid == zeroid )
        return MakeResultError("invalid parameter");

    CPubKey mypk;
    SET_MYPK_OR_REMOTE(mypk, remotepk);
    result = CancelBuyOffer<T, A>(mypk, 0,tokenid,bidtxid);
    RETURN_IF_ERROR(CCerror);
    return result;
}

UniValue tokencancelbid(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokencancelbid<TokensV1, AssetsV1>("tokencancelbid", params, fHelp, remotepk);
}
UniValue tokenv2cancelbid(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokencancelbid<TokensV2, AssetsV2>("tokenv2cancelbid", params, fHelp, remotepk);
}

template <class T, class A>
UniValue tokenfillbid(const std::string& name, const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    UniValue result(UniValue::VOBJ); 
    int64_t fillamount; 
    std::string hex; 
    uint256 tokenid,bidtxid;

    CCerror.clear();

    if (fHelp || params.size() != 3 && params.size() != 4)
        throw runtime_error(name + " tokenid bidtxid fillamount [unit_price]\n");
    if (ensure_CCrequirements(A::EvalCode()) < 0 || ensure_CCrequirements(T::EvalCode()) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);

    if (!remotepk.IsValid() && !EnsureWalletIsAvailable(false))
        throw runtime_error("wallet is required");
    CONDITIONAL_LOCK2(cs_main, pwalletMain->cs_wallet, !remotepk.IsValid());
    
    tokenid = Parseuint256((char *)params[0].get_str().c_str());
    bidtxid = Parseuint256((char *)params[1].get_str().c_str());
    fillamount = atoll(params[2].get_str().c_str());		
    if (fillamount <= 0)
        return MakeResultError("fillamount must be positive");
          
    if (tokenid == zeroid || bidtxid == zeroid)
        return MakeResultError("must provide tokenid and bidtxid");
    
    CAmount unit_price = 0LL;
    if (params.size() == 4)
	    unit_price = AmountFromValue(params[3].get_str().c_str());
    CPubKey mypk;
    SET_MYPK_OR_REMOTE(mypk, remotepk);

    result = FillBuyOffer<T, A>(mypk, 0, tokenid, bidtxid, fillamount, unit_price);
    RETURN_IF_ERROR(CCerror);
    return result;
}

UniValue tokenfillbid(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokenfillbid<TokensV1, AssetsV1>("tokenfillbid", params, fHelp, remotepk);
}
UniValue tokenv2fillbid(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokenfillbid<TokensV2, AssetsV2>("tokenv2fillbid", params, fHelp, remotepk);
}

template <class T, class A>
UniValue tokenask(const std::string& name, const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    UniValue result(UniValue::VOBJ); 
    int64_t askamount, numtokens; 
    std::string hex; 
    uint256 tokenid;

    CCerror.clear();
    if (fHelp || params.size() < 3 || params.size() > 4)
        throw runtime_error(name + " numtokens tokenid price [expiry-height]\n");
    if (ensure_CCrequirements(A::EvalCode()) < 0 || ensure_CCrequirements(T::EvalCode()) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
    
    if (!remotepk.IsValid() && !EnsureWalletIsAvailable(false))
        throw runtime_error("wallet is required");
    CONDITIONAL_LOCK2(cs_main, pwalletMain->cs_wallet, !remotepk.IsValid());

	numtokens = atoll(params[0].get_str().c_str());			
    tokenid = Parseuint256((char *)params[1].get_str().c_str());
    CAmount price = AmountFromValue(params[2]);
    askamount = (price * numtokens);
    if (tokenid == zeroid) 
        return MakeResultError("tokenid invalid");
    if (numtokens <= 0)
        return MakeResultError("numtokens invalid");
    if (price <= 0)
        return MakeResultError("price invalid");
    if (askamount <= 0)
        return MakeResultError("askamount invalid");

    int32_t expiryHeight; 
    {
        LOCK(cs_main);
        expiryHeight = chainActive.Height() + 4 * 7 * 24 * 60; // 4 weeks for blocktime 60 sec
    }
    if (params.size() == 4) {
        expiryHeight = atol(params[3].get_str().c_str());		
        if (!remotepk.IsValid() && expiryHeight < chainActive.LastTip()->GetHeight())
            return MakeResultError("expiry height invalid");
    }

    CPubKey mypk;
    SET_MYPK_OR_REMOTE(mypk, remotepk);
    result = CreateSell<T, A>(mypk, 0, numtokens, tokenid, askamount, expiryHeight);
    RETURN_IF_ERROR(CCerror);    
    return result;
}

UniValue tokenask(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokenask<TokensV1, AssetsV1>("tokenask", params, fHelp, remotepk);
}
UniValue tokenv2ask(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokenask<TokensV2, AssetsV2>("tokenv2ask", params, fHelp, remotepk);
}

// not implemented
UniValue tokenswapask(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    static uint256 zeroid;
    UniValue result(UniValue::VOBJ); int64_t askamount,numtokens; std::string hex; double price; uint256 tokenid,otherid;

    CCerror.clear();
    if (fHelp || params.size() != 4)
        throw runtime_error("tokenswapask numtokens tokenid otherid price\n");
    if (ensure_CCrequirements(EVAL_ASSETS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);

    if (!EnsureWalletIsAvailable(false))
        throw runtime_error("wallet is required");
    CONDITIONAL_LOCK2(cs_main, pwalletMain->cs_wallet, !remotepk.IsValid());

    throw runtime_error("tokenswapask not supported\n");

    /*
	numtokens = atoll(params[0].get_str().c_str());			
    tokenid = Parseuint256((char *)params[1].get_str().c_str());
    otherid = Parseuint256((char *)params[2].get_str().c_str());
    price = atof(params[3].get_str().c_str());
    askamount = (price * numtokens);
    hex = CreateSwap<TokensV2, AssetsV2>(0,numtokens,tokenid,otherid,askamount);
    RETURN_IF_ERROR(CCerror);
    if (price > 0 && numtokens > 0) {
        if (hex.size() > 0) {
            result.push_back(Pair("result", "success"));
            result.push_back(Pair("hex", hex));
        } else
            ERR_RESULT("couldnt create swap");
    } else {
        ERR_RESULT("price and numtokens must be positive");
    }
    */
    return result;
}

template <class T, class A>
UniValue tokencancelask(const std::string& name, const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    UniValue result(UniValue::VOBJ); std::string hex; int32_t i; uint256 tokenid,asktxid;

    CCerror.clear();
    if (fHelp || params.size() != 2)
        throw runtime_error(name + " tokenid asktxid\n");

    if (ensure_CCrequirements(A::EvalCode()) < 0 || ensure_CCrequirements(T::EvalCode()) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);

    if (!EnsureWalletIsAvailable(false))
        throw runtime_error("wallet is required");
    CONDITIONAL_LOCK2(cs_main, pwalletMain->cs_wallet, !remotepk.IsValid());

    tokenid = Parseuint256((char *)params[0].get_str().c_str());
    asktxid = Parseuint256((char *)params[1].get_str().c_str());
    if (tokenid == zeroid || asktxid == zeroid)
        return MakeResultError("invalid parameter");

    CPubKey mypk;
    SET_MYPK_OR_REMOTE(mypk, remotepk);
    result = CancelSell<T, A>(mypk, 0, tokenid, asktxid);
    RETURN_IF_ERROR(CCerror);
    return(result);
}

UniValue tokencancelask(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokencancelask<TokensV1, AssetsV1>("tokencancelask", params, fHelp, remotepk);
}
UniValue tokenv2cancelask(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokencancelask<TokensV2, AssetsV2>("tokenv2cancelask", params, fHelp, remotepk);
}

template <class T, class A>
UniValue tokenfillask(const std::string& name, const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    UniValue result(UniValue::VOBJ); 
    int64_t fillunits; 
    std::string hex; 
    uint256 tokenid,asktxid;

    if (fHelp || params.size() != 3 && params.size() != 4)
        throw runtime_error(name + " tokenid asktxid fillunits [unitprice]\n");
    if (ensure_CCrequirements(A::EvalCode()) < 0 || ensure_CCrequirements(T::EvalCode()) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);

    if (!remotepk.IsValid() && !EnsureWalletIsAvailable(false))
        throw runtime_error("wallet is required");
    CONDITIONAL_LOCK2(cs_main, pwalletMain->cs_wallet, !remotepk.IsValid());

    tokenid = Parseuint256((char *)params[0].get_str().c_str());
    asktxid = Parseuint256((char *)params[1].get_str().c_str());
	fillunits = atoll(params[2].get_str().c_str());	 
    if (fillunits <= 0)
        return MakeResultError("fillunits must be positive");
    if (tokenid == zeroid || asktxid == zeroid)
        return MakeResultError("invalid parameter");
    CAmount unit_price = 0LL;
    if (params.size() == 4)
	    unit_price = AmountFromValue(params[3].get_str().c_str());	 

    CPubKey mypk;
    SET_MYPK_OR_REMOTE(mypk, remotepk);
    result = FillSell<T, A>(mypk, 0, tokenid, asktxid, fillunits, unit_price);
    RETURN_IF_ERROR(CCerror);
    return result;
}

UniValue tokenfillask(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokenfillask<TokensV1, AssetsV1>("tokenfillask", params, fHelp, remotepk);
}
UniValue tokenv2fillask(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    return tokenfillask<TokensV2, AssetsV2>("tokenv2fillask", params, fHelp, remotepk);
}

// not used yet
UniValue tokenfillswap(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    static uint256 zeroid;
    UniValue result(UniValue::VOBJ); 
    int64_t fillunits; std::string hex; uint256 tokenid,otherid,asktxid;

    CCerror.clear();
    if (fHelp || params.size() != 4 && params.size() != 5)
        throw runtime_error("tokenfillswap tokenid otherid asktxid fillunits [unitprice]\n");
    if (!remotepk.IsValid() && ensure_CCrequirements(EVAL_ASSETS) < 0)
        throw runtime_error(CC_REQUIREMENTS_MSG);
        
    if (!EnsureWalletIsAvailable(false))
        throw runtime_error("wallet is required");
    CONDITIONAL_LOCK2(cs_main, pwalletMain->cs_wallet, !remotepk.IsValid());

    throw runtime_error("tokenfillswap not supported\n");

    tokenid = Parseuint256((char *)params[0].get_str().c_str());
    otherid = Parseuint256((char *)params[1].get_str().c_str());
    asktxid = Parseuint256((char *)params[2].get_str().c_str());
    //fillunits = atol(params[3].get_str().c_str());
	fillunits = atoll(params[3].get_str().c_str());  // dimxy changed to prevent loss of significance
    CAmount unit_price = 0LL;
    if (params.size() == 5)
	    unit_price = AmountFromValue(params[4].get_str().c_str());
    CPubKey mypk;
    SET_MYPK_OR_REMOTE(mypk, remotepk);
    result = FillSell<TokensV1, AssetsV1>(mypk, 0, tokenid, asktxid, fillunits, unit_price);
    RETURN_IF_ERROR(CCerror);
    if (fillunits > 0) {
        if ( hex.size() > 0 ) {
            result.push_back(Pair("result", "success"));
            result.push_back(Pair("hex", hex));
        } else ERR_RESULT("couldnt fill bid");
    } else {
        ERR_RESULT("fillunits must be positive");
    }
    return result;
}

UniValue addccv2signature(const UniValue& params, bool fHelp, const CPubKey& remotepk)
{
    UniValue result(UniValue::VOBJ), jsonParams(UniValue::VOBJ); 

    CCerror.clear();
    if (fHelp || params.size() != 2)
        throw runtime_error("addccv2signature hextx '[ { \"ccaddress\": \"address\", \"condition\":\"cond-in-json\" },.. ]' \n");
    if (!IsCryptoConditionsEnabled())
        throw runtime_error("cc not enabled");
        
    if (!EnsureWalletIsAvailable(false))
        throw runtime_error("wallet is required");
    LOCK2(cs_main, pwalletMain->cs_wallet);
    
    vuint8_t vtx = ParseHex(params[0].get_str().c_str());
    if (params[1].getType() == UniValue::VARR)
        jsonParams = params[1].get_array();
    else if (params[1].getType() == UniValue::VSTR)  // json in quoted string '{...}'
        jsonParams.read(params[1].get_str().c_str());
    if (jsonParams.getType() != UniValue::VARR)
        throw runtime_error("parameter 2 must be array\n");

    result = AddSignatureCCTxV2(vtx, jsonParams);
    return result;
}


static const CRPCCommand commands[] =
{ //  category              name                actor (function)        okSafeMode
  //  -------------- ------------------------  -----------------------  ----------
     // tokens & assets
	{ "tokens",       "assetsaddress",    &assetsaddress,      true },
	{ "tokens v2",       "assetsv2address",    &assetsv2address,      true },

    { "tokens",       "tokeninfo",        &tokeninfo,         true },
    { "tokens v2",       "tokenv2info",      &tokenv2info,         true },
    { "tokens",       "tokenlist",        &tokenlist,         true },
    { "tokens v2",       "tokenv2list",      &tokenv2list,         true },
    { "tokens",       "tokenorders",      &tokenorders,       true },
    { "tokens v2",       "tokenv2orders",      &tokenv2orders,       true },
    { "tokens",       "mytokenorders",    &mytokenorders,     true },
    { "tokens v2",       "mytokenv2orders",    &mytokenv2orders,     true },
    { "tokens",       "tokenaddress",     &tokenaddress,      true },
    { "tokens v2",       "tokenv2address",   &tokenv2address,      true },
    { "tokens",       "tokenbalance",     &tokenbalance,      true },
    { "tokens v2",       "tokenv2balance",   &tokenv2balance,      true },
    { "tokens",       "tokencreate",      &tokencreate,       true },
    { "tokens v2",       "tokenv2create",    &tokenv2create,       true },
    { "tokens",       "tokentransfer",    &tokentransfer,     true },
    { "tokens v2",       "tokenv2transfer",     &tokenv2transfer,     true },
    { "tokens",       "tokentransfermany",   &tokentransfermany,     true },
    { "tokens v2",       "tokenv2transfermany", &tokenv2transfermany,     true },
    { "tokens",       "tokenbid",         &tokenbid,          true },
    { "tokens",       "tokencancelbid",   &tokencancelbid,    true },
    { "tokens",       "tokenfillbid",     &tokenfillbid,      true },
    { "tokens",       "tokenask",         &tokenask,          true },
    //{ "tokens",       "tokenswapask",     &tokenswapask,      true },
    { "tokens",       "tokencancelask",   &tokencancelask,    true },
    { "tokens",       "tokenfillask",     &tokenfillask,      true },
    { "tokens v2",       "tokenv2bid",         &tokenv2bid,          true },
    { "tokens v2",       "tokenv2cancelbid",   &tokenv2cancelbid,    true },
    { "tokens v2",       "tokenv2fillbid",     &tokenv2fillbid,      true },
    { "tokens v2",       "tokenv2ask",         &tokenv2ask,          true },
    //{ "tokens",       "tokenswapask",     &tokenswapask,      true },
    { "tokens v2",       "tokenv2cancelask",   &tokenv2cancelask,    true },
    { "tokens v2",       "tokenv2fillask",     &tokenv2fillask,      true },
    //{ "tokens",       "tokenfillswap",    &tokenfillswap,     true },
    { "tokens",       "tokenconvert", &tokenconvert, true },
    { "ccutils",       "addccv2signature", &addccv2signature, true },
    { "tokens",       "tokencreatetokel",      &tokencreatetokel,       true },
    { "tokens v2",       "tokenv2createtokel",    &tokenv2createtokel,       true },
    { "tokens",       "tokeninfotokel",        &tokeninfotokel,         true },
    { "tokens v2",       "tokenv2infotokel",      &tokenv2infotokel,         true },
};

void RegisterTokensRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
