--------------------------------------------------------------------------------
-- Project      : Opallios
--------------------------------------------------------------------------------
-- File         : matrix_pkg.vhd
-- Generated    : 06/13/2022
--------------------------------------------------------------------------------
-- Description  : package for data type for 64x64 LED matrix for sim
--------------------------------------------------------------------------------
-- Revision History :
--   Date        Author        Changes
--   06/13/2022  Matt D Smith  Initial Design
--------------------------------------------------------------------------------
library ieee;
    use ieee.std_logic_1164.all;
    use ieee.numeric_std.all;

package matrix_pkg is
    type t_matrix is array (63 downto 0) of std_logic_vector(63 downto 0);
    type t_RGB_matrix is array (2 downto 0) of t_matrix;
end package ;