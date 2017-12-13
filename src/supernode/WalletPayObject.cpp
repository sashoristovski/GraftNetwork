#include "WalletPayObject.h"
#include "graft_defines.h"
#include "WalletProxy.h"

void supernode::WalletPayObject::Owner(WalletProxy* o) { m_Owner = o; }

bool supernode::WalletPayObject::Init(const RTA_TransactionRecordBase& src) {
	bool ret = _Init(src);
    m_Status = ret ? NTransactionStatus::Success : NTransactionStatus::Fail;
	return ret;
}

bool supernode::WalletPayObject::_Init(const RTA_TransactionRecordBase& src) {
	BaseRTAObject::Init(src);

	// we allready have block num
	TransactionRecord.AuthNodes = m_Servant->GetAuthSample( TransactionRecord.BlockNum );
	if( TransactionRecord.AuthNodes.empty() ) return false;

	InitSubnet();

	vector<rpc_command::WALLET_PROXY_PAY::response> outv;
	rpc_command::WALLET_PROXY_PAY::request inbr;
	rpc_command::ConvertFromTR(inbr, TransactionRecord);
	if( !m_SubNetBroadcast.Send(dapi_call::WalletProxyPay, inbr, outv) || outv.empty() ) return false;


	if( outv.size()!=m_Servant->AuthSampleSize() ) return false;// not all signs gotted
	for(auto& a : outv) {
		if( !CheckSign(a.FSN_StakeWalletAddr, a.Sign) ) return false;
		m_Signs.push_back(a.Sign);
	}


	if( !PutTXToPool() ) return false;

	rpc_command::WALLET_PUT_TX_IN_POOL::request req;
	req.PaymentID = TransactionRecord.PaymentID;
	req.TransactionPoolID = m_TransactionPoolID;
	//LOG_PRINT_L5("PaymentID: "<<TransactionRecord.PaymentID);

	vector<rpc_command::WALLET_PUT_TX_IN_POOL::response> vv_out;

	if( !m_SubNetBroadcast.Send( dapi_call::WalletPutTxInPool, req, vv_out) ) return false;


	ADD_RTA_OBJECT_HANDLER(GetPayStatus, rpc_command::WALLET_GET_TRANSACTION_STATUS, WalletPayObject);


	return true;
}

bool supernode::WalletPayObject::GetPayStatus(const rpc_command::WALLET_GET_TRANSACTION_STATUS::request& in, rpc_command::WALLET_GET_TRANSACTION_STATUS::response& out) {
	out.Status = int(m_Status);
	//TimeMark -= boost::posix_time::hours(3);
    out.Result = STATUS_OK;
    return true;
}




bool supernode::WalletPayObject::PutTXToPool() {
	// TODO: IMPL. all needed data we have in TransactionRecord + m_Signs.
	// TODO: Result, monero_tranaction_id must be putted to m_TransactionPoolID
	return true;
}

