import grid from "bootstrap-icons/icons/grid-1x2.svg";
import box from "bootstrap-icons/icons/box-seam.svg";
import clock from "bootstrap-icons/icons/clock-history.svg";
import chart from "bootstrap-icons/icons/bar-chart-line.svg";
import settings from "bootstrap-icons/icons/sliders.svg";
import leaf from "bootstrap-icons/icons/leaf.svg";
import check from "bootstrap-icons/icons/check-circle.svg";
import bell from "bootstrap-icons/icons/bell.svg";
import refresh from "bootstrap-icons/icons/arrow-clockwise.svg";
import plus from "bootstrap-icons/icons/plus-lg.svg";
import wallet from "bootstrap-icons/icons/wallet2.svg";

const icons = { overview: grid, products: box, history: clock, analytics: chart, settings, leaf, check, bell, refresh, plus, wallet };
export function Icon({ name }) {
  return <img className="ui-icon" src={icons[name]} alt="" />;
}
