module counter (
  input  logic       clk,
  input  logic       rst,
  input  logic       ce,
  output logic [7:0] counter
);
  always_ff @(posedge clk or posedge rst) begin
    if (rst)
      counter <= 8'd0;
    else if (ce)
      counter <= counter + 1;
  end
endmodule
