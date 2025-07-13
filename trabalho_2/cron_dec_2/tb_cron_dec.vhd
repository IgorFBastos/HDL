library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity tb_cron_dec is
end tb_cron_dec;

architecture sim of tb_cron_dec is

    signal clock  : std_logic := '1';
    signal reset  : std_logic;
    signal carga  : std_logic;
    signal conta  : std_logic;
    signal chaves : std_logic_vector(6 downto 0);

begin

    -- Geração do clock
    clock <= not clock after 10 ns;

    -- Estímulos
    reset  <= '1', '0' after 73 ns;
    carga  <= '0', '1' after 133 ns, '0' after 425 ns;
    conta  <= '0', '1' after 543 ns, '0' after 925 ns;
    chaves <= "0000101";

    -- Instanciação da unidade sob teste (UUT)
    uut : entity work.cron_dec
        generic map (
            CLOCK_FREQ => 4  -- Para simulação, usar valor pequeno
        )
        port map (
            clock   => clock,
            reset   => reset,
            carga   => carga,
            conta   => conta,
            chaves  => chaves,
            an      => open,
				dec_ddp => open

        );

end sim;