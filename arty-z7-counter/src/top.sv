module top (
  input  logic       clk,
  input  logic       rst,
  output logic [3:0] led
);
  logic [22:0] clk_div;
  logic [7:0]  counter_val;

  always_ff @(posedge clk or posedge rst) begin
    if (rst)
      clk_div <= 0;
    else
      clk_div <= clk_div + 1;
  end

  counter u_counter (
    .clk(clk),
    .rst(rst),
    .ce (clk_div == 0),
    .counter(counter_val)
  );

  assign led = counter_val[7:4];
endmodule