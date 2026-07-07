`timescale 1 ns / 1 ps

module custom_axi_lite #(
    parameter integer C_S_AXI_DATA_WIDTH = 32,
    parameter integer C_S_AXI_ADDR_WIDTH = 6
) (
    input  wire                              S_AXI_ACLK,
    input  wire                              S_AXI_ARESETN,
    input  wire [C_S_AXI_ADDR_WIDTH-1:0]     S_AXI_AWADDR,
    input  wire [2:0]                        S_AXI_AWPROT,
    input  wire                              S_AXI_AWVALID,
    output wire                              S_AXI_AWREADY,
    input  wire [C_S_AXI_DATA_WIDTH-1:0]     S_AXI_WDATA,
    input  wire [(C_S_AXI_DATA_WIDTH/8)-1:0] S_AXI_WSTRB,
    input  wire                              S_AXI_WVALID,
    output wire                              S_AXI_WREADY,
    output reg  [1:0]                        S_AXI_BRESP,
    output reg                               S_AXI_BVALID,
    input  wire                              S_AXI_BREADY,
    input  wire [C_S_AXI_ADDR_WIDTH-1:0]     S_AXI_ARADDR,
    input  wire [2:0]                        S_AXI_ARPROT,
    input  wire                              S_AXI_ARVALID,
    output wire                              S_AXI_ARREADY,
    output reg  [C_S_AXI_DATA_WIDTH-1:0]     S_AXI_RDATA,
    output reg  [1:0]                        S_AXI_RRESP,
    output reg                               S_AXI_RVALID,
    input  wire                              S_AXI_RREADY
);

    localparam [31:0] REG_ID      = 32'hA710_0007;
    localparam [31:0] REG_STATUS  = 32'h5A5A_A5A5;

    reg [31:0] scratch;
    reg [31:0] counter;
    reg [C_S_AXI_ADDR_WIDTH-1:0] read_addr;

    wire write_accept = S_AXI_AWVALID && S_AXI_WVALID && !S_AXI_BVALID;
    wire read_accept = S_AXI_ARVALID && !S_AXI_RVALID;
    wire [3:0] write_reg = S_AXI_AWADDR[5:2];
    wire [3:0] read_reg = S_AXI_ARADDR[5:2];

    assign S_AXI_AWREADY = write_accept;
    assign S_AXI_WREADY = write_accept;
    assign S_AXI_ARREADY = read_accept;

    always @(posedge S_AXI_ACLK) begin
        if (!S_AXI_ARESETN) begin
            scratch <= 32'h0000_0000;
            counter <= 32'h0000_0000;
            S_AXI_BRESP <= 2'b00;
            S_AXI_BVALID <= 1'b0;
        end else begin
            counter <= counter + 32'h0000_0001;

            if (write_accept) begin
                if (write_reg == 4'h1) begin
                    if (S_AXI_WSTRB[0]) scratch[7:0] <= S_AXI_WDATA[7:0];
                    if (S_AXI_WSTRB[1]) scratch[15:8] <= S_AXI_WDATA[15:8];
                    if (S_AXI_WSTRB[2]) scratch[23:16] <= S_AXI_WDATA[23:16];
                    if (S_AXI_WSTRB[3]) scratch[31:24] <= S_AXI_WDATA[31:24];
                end
                S_AXI_BRESP <= 2'b00;
                S_AXI_BVALID <= 1'b1;
            end else if (S_AXI_BVALID && S_AXI_BREADY) begin
                S_AXI_BVALID <= 1'b0;
            end
        end
    end

    always @(posedge S_AXI_ACLK) begin
        if (!S_AXI_ARESETN) begin
            read_addr <= {C_S_AXI_ADDR_WIDTH{1'b0}};
            S_AXI_RDATA <= 32'h0000_0000;
            S_AXI_RRESP <= 2'b00;
            S_AXI_RVALID <= 1'b0;
        end else begin
            if (read_accept) begin
                read_addr <= S_AXI_ARADDR;
                case (read_reg)
                4'h0: S_AXI_RDATA <= REG_ID;
                4'h1: S_AXI_RDATA <= scratch;
                4'h2: S_AXI_RDATA <= counter;
                4'h3: S_AXI_RDATA <= scratch ^ REG_STATUS;
                default: S_AXI_RDATA <= 32'h0000_0000;
                endcase
                S_AXI_RRESP <= 2'b00;
                S_AXI_RVALID <= 1'b1;
            end else if (S_AXI_RVALID && S_AXI_RREADY) begin
                S_AXI_RVALID <= 1'b0;
            end
        end
    end

    wire unused = |{S_AXI_AWPROT, S_AXI_ARPROT, read_addr};

endmodule
