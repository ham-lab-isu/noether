#include <noether_gui/widgets/angle_double_spin_box.h>

#include <QTextStream>
#include <QValidator>
#include <regex>
#include <cmath>
#include <algorithm>

/**
 * @brief Helper function for splitting text into a value and unit
 */
static std::tuple<bool, double, std::string> splitSafe(const std::string& text)
{
  static const std::regex re(
      R"(^\s*([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)\s*([[:alpha:]°]*)\s*$)");
  std::smatch m;
  if (!std::regex_match(text, m, re)) return {false, 0.0, ""};

  double v = 0.0;
  try { v = std::stod(m.str(1)); }
  catch (...) { return {false, 0.0, ""}; } // never propagate exceptions

  std::string unit = m.str(2);
  std::transform(unit.begin(), unit.end(), unit.begin(), ::tolower);
  return {true, v, unit};
}

namespace noether
{
AngleDoubleSpinBox::AngleDoubleSpinBox(QWidget* parent) : QDoubleSpinBox(parent)
{
  setRange(-M_PI, M_PI);
  setSingleStep(1.0 * M_PI / 180.0);
  setDecimals(4);
  unit_ = "rad";
}

QString AngleDoubleSpinBox::textFromValue(double value_rad) const
{
  double value;

  const bool as_deg = (unit_ == "deg" || unit_ == "°");
  const double shown = as_deg ? (value_rad * 180.0 / M_PI) : value_rad;

  QString text;
  QTextStream stream(&text);
  stream.setRealNumberPrecision(decimals());
  stream << shown;
  if (!unit_.empty())
    stream << " " << QString::fromStdString(unit_);
  return text;
}

double AngleDoubleSpinBox::valueFromText(const QString& text) const
{
  auto [ok, v, unit] = splitSafe(text.toStdString());
  if (!ok)
  {
    // During partial edits (".", "-", "1e"), keep the current value instead of throwing
    return this->value();
  }

  // If there is no unit defined, assume the last known unit is still active for the sake of conversion
  if (unit.empty())
    unit = unit_;

  // Convert the value to meters for internal storage
  double rad = 0.0;
  if (unit == "rad" || unit.empty())
    rad = v;
  else if (unit == "deg" || unit == "°")
    rad = v * M_PI / 180.0;
  else
    return this->value();

  // Update the unit
  unit_ = unit;

  return rad;
}

  QValidator::State AngleDoubleSpinBox::validate(QString& input, int& /*pos*/) const
{
  const QString s = input.trimmed();

  // Allow common "editing" states as Intermediate
  static const QRegularExpression partialRe(
      R"(^[+-]?$|^[+-]?\.$|^[+-]?\d+\.$|^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?)?$)");
  if (s.isEmpty() || partialRe.match(s.section(' ', 0, 0)).hasMatch())
    return QValidator::Intermediate;

  auto [ok, v, unit] = splitSafe(s.toStdString());
  if (!ok) return QValidator::Invalid;

  if (unit.empty()) unit = unit_;
  double rad = (unit == "deg" || unit == "°") ? (v * M_PI / 180.0) : v;

  if (rad < minimum() || rad > maximum()) return QValidator::Invalid;
  return QValidator::Acceptable;
}
}  // namespace noether
