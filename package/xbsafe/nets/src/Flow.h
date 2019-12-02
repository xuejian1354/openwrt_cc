
typedef struct {
	unsigned long local_rx_start;
	unsigned long local_rx_end;
	unsigned long local_tx_start;
	unsigned long local_tx_end;
	unsigned long wan_rx_start;
	unsigned long wan_rx_end;
	unsigned long wan_tx_start;
	unsigned long wan_tx_end;
	unsigned long wan_packet_rx_start;
	unsigned long wan_packet_rx_end;	
	unsigned long wan_packet_rx_droped_start;
	unsigned long wan_packet_rx_droped_end;		
}Flow_Struct;

class NetFlow{
	char wan[128];
	char local[128];
	TElapsed tp;
	Flow_Struct flow;
	int sysfs_exist;
	void sysflow_start(void);
	void sysflow_stop(void);
	void nlflow_start(void);
	void nlflow_stop(void);

	void compute(void);
public:
	unsigned long local_rx_bytes;
    unsigned long local_tx_bytes;
    unsigned long wan_rx_bytes;
    unsigned long wan_tx_bytes;

    int time_interval;

	NetFlow();
	void start(void);
	void stop(void);
	void checkpoint(void);
	void checkpoint_nl(void);
	void checkpoint_sys(void);

	unsigned long getWanBandwidth(void);
	unsigned long getLocalBandwidth(void);
	unsigned long getWanBytes(void);
	unsigned long getWanRxPackets(void);
	unsigned long getWanRxDropedPackets(void);
};