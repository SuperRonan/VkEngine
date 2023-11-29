#pragma once

#include <Core/Maths/Types.hpp>
#include <GLFW/glfw3.h>

namespace vkl
{
	class MouseHandler
	{
	public:

		enum class Mode {Position, Direction};
	
	protected:

		Mode m_mode;

		Vector2d m_prev_pos, m_current_pos;
		Vector2d m_delta;

		GLFWwindow* m_window;
		
		int m_prev_buttons[5];
		int m_current_buttons[5];

		double m_scroll;

		// in degrees
		double m_pitch, m_yaw;

		void updatePhiTheta();

		static double s_scroll;

		static void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);


	public:

		float fov;
		
		double sensibility;

		MouseHandler(GLFWwindow * window, Mode mode = Mode::Direction, double sensibility=1.0);

		MouseHandler(MouseHandler const&) = delete;
		MouseHandler(MouseHandler &&) = delete;

		void update(double dt);

		template <class Float>
		Vector2<Float> currentPosition()const
		{
			Vector2<Float> res;
			res.x = m_current_pos.x;
			res.y = m_current_pos.y;
			return res;
		}

		template <class Float>
		Vector2<Float> deltaPosition()const
		{
			Vector2<Float> res;
			res.x = m_delta.x;
			res.y = m_delta.y;
			return res;
		}

		template <class Float>
		Vector3<Float> direction()const
		{
			Vector3<Float> res;
			double r_yaw = glm::radians(m_yaw), r_pitch = -glm::radians(m_pitch);
			res.x = std::cos(r_yaw) * std::cos(r_pitch);
			res.y = std::sin(r_pitch);
			res.z = std::sin(r_yaw) * std::cos(r_pitch);
			return res;
		}

		double getScroll()const;

		bool inWindow()const;


		void setMode(Mode mode);

		bool isButtonCurrentlyPressed(int id)const;

		bool isButtonJustPressed(int id)const;

		bool isButtonJustReleased(int id)const;

		template <class Out>
		Out& print(Out& out)const
		{
			out << "pos: (" << m_current_pos.x << ", "<<m_current_pos.y<<"), " <<
				"delta: (" << m_delta.x <<", "<<m_delta.y << "), pitch: "<<m_pitch<<", yaw: "<<m_yaw<<"\n";
			return out;
		}
	};
}